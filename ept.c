#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <asm/special_insns.h>
#include <linux/device.h>
#include <linux/fs.h>

#include "ept.h"
#include "vmm.h"
#include "vmxasm.h"
#include "exit.h"

int ept_check_features()
{
    union __ia32_vmx_ept_vpid_cap_register vpid_register; 
    union __ia32_mtrr_def_type_register mtrr_def_type;

    vpid_register.flags = native_read_msr(MSR_IA32_VMX_EPT_VPID_CAP);
    mtrr_def_type.flags = native_read_msr(MSR_MTRRdefType);

    if (
        !vpid_register.bits.page_walk_length4
        || !vpid_register.bits.memory_type_writeback 
        || !vpid_register.bits.pde_2mb_pages
       )
    {
        return -1;
    }

    if ( !vpid_register.bits.advanced_vmexit_ept_violation_information)
    {
        printk(
            KERN_INFO 
            "Processor does not support advanced " 
            "advanced_vmexit_ept_violation_information\n"
        );
    }

    if (!mtrr_def_type.bits.mtrr_enable)
    {
        printk(
            KERN_INFO 
            "MTRR dynamic ranges not supported\n" 
        );
        return -1;
    }
    printk(KERN_INFO "All EPT features available");
    return 0;
}

int ept_build_mtrr_map(struct vmm_ctx* vmm_context)
{
    union __ia32_mtrr_capabilities_register mtrr_cap;
    union __ia32_mtrr_physbase_register curr_phys_base;
    union __ia32_mtrr_physmask_register curr_phys_mask;

    struct __mtrr_range_descriptor *descriptor;

    uint64_t current_reg;
    uint64_t number_of_bits_in_mask;

    mtrr_cap.flags = native_read_msr(MSR_MTRRcap);

    printk(
        KERN_INFO 
        "Number or dynamic ranges: %d\n", 
        mtrr_cap.bits.variable_range_count
    );

    if (
        mtrr_cap.bits.variable_range_count 
        > 
        (sizeof(vmm_context->memory_ranges) 
        / 
        sizeof(struct __mtrr_range_descriptor)
        ))
        {
            printk(
                KERN_INFO 
                "Not enough space to store dynamic ranges\n"
            );
            return -1;
        }

    for ( 
        current_reg = 0; 
        current_reg < mtrr_cap.bits.variable_range_count ; 
        current_reg++)
    {
        curr_phys_base.flags = 
            native_read_msr(IA32_MTRR_PHYSBASE0 + current_reg * 2);
        curr_phys_mask.flags = 
            native_read_msr(IA32_MTRR_PHYSMASK0 + current_reg * 2);

        if (curr_phys_mask.bits.valid)
        {
            descriptor = 
                &vmm_context
                ->memory_ranges[vmm_context->enabled_memory_ranges++];

            descriptor->physical_base_address = 
                curr_phys_base.bits.page_frame_number * PAGE_SIZE;

            number_of_bits_in_mask = __bit_scan_forward(
                curr_phys_mask.bits.page_frame_number * PAGE_SIZE
            );

            descriptor->physical_end_address = 
                descriptor->physical_base_address + 
                (1ULL << number_of_bits_in_mask) - 1;

            descriptor->memory_type = curr_phys_base.bits.type;

            if (descriptor->memory_type == MEMORY_TYPE_WRITEBACK)
            {
                vmm_context->enabled_memory_ranges--;
            }

            printk(
                KERN_INFO
                "MTRR RANGE: Base=0x%llx End=0x%llx Type=0x%x",
                descriptor->physical_base_address,
                descriptor->physical_end_address,
                descriptor->memory_type
            );
        }
    }

    printk(
        KERN_INFO
        "MTRR RANGE: %lld ranges commited",
        vmm_context->enabled_memory_ranges
    );

    return 0;
}

void setup_pml2_entry(
    struct vmm_ctx* vmm_context, 
    union ept_pml2_entry* entry, 
    uint64_t page_frame_number
)
{
    uint64_t addr_of_page, current_mtrr_range, target_memory_type;

    entry->bits.page_frame_number = page_frame_number;    

    // page_frame_number * 2Mb
    addr_of_page = page_frame_number * (512 * PAGE_SIZE);

    // See gbhv
    if (page_frame_number == 0)
    {
        entry->bits.memory_type = MEMORY_TYPE_UNCACHEABLE;
    }

    target_memory_type = MEMORY_TYPE_WRITEBACK;
    for (
        current_mtrr_range = 0 ; 
        current_mtrr_range < vmm_context->enabled_memory_ranges;
        current_mtrr_range++)
    {
        if ((
            addr_of_page <= 
            vmm_context->
                memory_ranges[current_mtrr_range].physical_end_address)
            &&
            ((addr_of_page + (512 * PAGE_SIZE) - 1) >
            vmm_context->
                memory_ranges[current_mtrr_range].physical_base_address))
        {
            target_memory_type =
                vmm_context->memory_ranges[current_mtrr_range].memory_type; 
            if (target_memory_type == MEMORY_TYPE_UNCACHEABLE)
            {
                break;
            }
        }
    }
    entry->bits.memory_type = target_memory_type;

}

struct ept_page_table* ept_create_identity_page_table
(
    struct vmm_ctx* vmm_context
)
{
    int i, j;
    union ept_pml3_entry template_entry_pml3;
    union ept_pml2_entry template_entry_pml2;

    struct ept_page_table* page_table = 
        (struct ept_page_table*) 
        kmalloc(sizeof(struct ept_page_table), GFP_ATOMIC | GFP_KERNEL);

    page_table->ept_pml4 = 0;
    page_table->ept_pml3 = 0;
    for ( i = 0 ; i < PML3_COUNT ; i++)
    {
        page_table->ept_pml2[i] = 0;
    }
    INIT_LIST_HEAD(&page_table->splited_pages);
    INIT_LIST_HEAD(&page_table->hooked_pages);

    // On this one, only the first entry will be used
    page_table->ept_pml4 = 
        (union ept_pml4_entry*)get_zeroed_page(GFP_KERNEL | GFP_ATOMIC);
    
    // On this one we use the 512 entries
    page_table->ept_pml3 = 
        (union ept_pml3_entry*)get_zeroed_page(GFP_KERNEL | GFP_ATOMIC);
    
    // There will be 512 * 512 entries on this one. So we need 512 pages
    // This assert that all pages have been allocated
    for (i = 0; i < PML3_COUNT ; i++)
    {
        page_table->ept_pml2[i] = 
            (union ept_pml2_entry*)get_zeroed_page(GFP_KERNEL | GFP_ATOMIC);
        if (page_table->ept_pml2[i] == NULL)
        {
            free_ept_page_table(page_table);
            return NULL;
        }
    }

    // Check that all pages have been allocated
    if (
        !page_table->ept_pml3 
        || !page_table->ept_pml4)
    {
        free_ept_page_table(page_table);
        return NULL;
    }


    page_table->ept_pml4[0].bits.read_access = 1;
    page_table->ept_pml4[0].bits.write_access = 1;
    page_table->ept_pml4[0].bits.execute_access = 1;
    page_table->ept_pml4[0].bits.page_frame_number = 
        __pa(&page_table->ept_pml3[0]) / PAGE_SIZE;


    template_entry_pml3.flags = 0;
    template_entry_pml3.bits.read_access = 1;
    template_entry_pml3.bits.write_access = 1;
    template_entry_pml3.bits.execute_access = 1;

    for (i = 0; i < PML3_COUNT ; i++)
    {
        page_table->ept_pml3[i].flags = template_entry_pml3.flags;
        page_table->ept_pml3[i].bits.page_frame_number = 
            __pa(&page_table->ept_pml2[i][0]) / PAGE_SIZE;
    }

    template_entry_pml2.flags = 0;
    template_entry_pml2.bits.read_access = 1;
    template_entry_pml2.bits.write_access = 1;
    template_entry_pml2.bits.execute_access = 1;
    template_entry_pml2.bits.large_page = 1;

    for (i = 0; i < PML3_COUNT ; i++)
    {
        for (j = 0; j < PML2_COUNT ; j++)
        {
            page_table->ept_pml2[i][j].flags = template_entry_pml2.flags;
            setup_pml2_entry(
                vmm_context, 
                &page_table->ept_pml2[i][j], 
                i * PML2_COUNT + j);
            //printk(KERN_INFO "PML2[%3x][%3x] = [%16llx] = %llx\n", i, j, (uint64_t)&page_table->ept_pml2[i][j], page_table->ept_pml2[i][j].flags); 
        }
    }

    return page_table;
};

void free_ept_page_table(struct ept_page_table* page_table)
{
    int i;
    struct list_head* current_page;
    struct page_in_list * p;
    // one page
    if (page_table->ept_pml4)
    {
        free_page((uint64_t)page_table->ept_pml4);
    }

    // one page
    if (page_table->ept_pml3)
    {
        free_page((uint64_t)page_table->ept_pml3);
    }

    for ( i = 0 ; i < PML3_COUNT; i++)
    {
        if ( page_table->ept_pml2[i] != 0 )
        {
            free_page((uint64_t)page_table->ept_pml2[i]);
        }
    }

    list_for_each(current_page, &page_table->splited_pages){
        p = list_entry(current_page, struct page_in_list, list);
        kfree(p);
    }

    list_for_each(current_page, &page_table->hooked_pages){
        p = list_entry(current_page, struct page_in_list, list);
        kfree(p);
    }
    
    kfree(page_table);
}

int ept_setup_proc(struct vcpu_ctx* vcpu)
{
    struct ept_page_table* page_table;
    union ept_pointer eptp;

    page_table = ept_create_identity_page_table(vcpu->vmm_context);
    if (page_table == NULL)
    {
        printk(KERN_ALERT "Can't allocate memory for EPT\n");
        return -1;
    }

    vcpu->ept_pt = page_table;

    eptp.flags = 0;
    eptp.bits.memory_type = MEMORY_TYPE_WRITEBACK;
    eptp.bits.enable_accessed_and_dirty_flags = 0;
    eptp.bits.page_walk_length = 3;
    eptp.bits.pml4_address = __pa(page_table->ept_pml4) / PAGE_SIZE;

    printk(
        KERN_INFO
        "EPT pml4 physical: %llx EPT pml4 virtual : %llx", 
        (uint64_t)eptp.bits.pml4_address,
        (uint64_t)page_table->ept_pml4);

    vcpu->eptp = eptp;
    return 0;
}

int ept_init(struct vmm_ctx* vmm_context)
{
    if (ept_check_features())
    {
        printk(
            KERN_ALERT "Processor does not support all EPT features\n"
        );
        return -1;
    }

    if (ept_build_mtrr_map(vmm_context))
    {
        printk(
            KERN_ALERT "Could not build MTRR map\n"
        );
        return -1;
    }
    return 0;
}
