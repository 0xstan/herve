#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/workqueue.h>
#include <asm/special_insns.h>
#include <linux/device.h>
#include <linux/fs.h>

#include "vcpu.h"
#include "vmm.h"
#include "vmcs.h"
#include "utils.h"
#include "vmxasm.h"

struct vcpu_ctx* allocate_vcpu()
{
    struct vcpu_ctx* vcpu = NULL; 

    vcpu = 
        (struct vcpu_ctx*) 
        kzalloc(
            sizeof(struct vcpu_ctx), 
            GFP_KERNEL | GFP_ATOMIC);

    if (!vcpu) 
    {
        printk(KERN_INFO "Cannot allocate vcpu\n");
        return NULL;
    }

    return vcpu;
}

void init_vcpu_structs(struct vcpu_ctx* vcpu)
{
    // init and clear regions
    vcpu->vmcs = (struct _vmcs*)__get_free_page(GFP_KERNEL | GFP_ATOMIC);
    vcpu->msr_bitmap = (char*)__get_free_page(GFP_KERNEL | GFP_ATOMIC);

    memset(vcpu->vmcs, 0, PAGE_SIZE);
    memset(vcpu->msr_bitmap, 0, PAGE_SIZE);

    vcpu->vmm_stack = 
        kzalloc(
            sizeof(struct vmm_stack), 
            GFP_KERNEL | GFP_ATOMIC
        );

    vcpu->vmm_stack->cpu = vcpu;

    // set pa address and vmcs headers
    vcpu->vmcs_physical = __pa(vcpu->vmcs);
    vcpu->vmcs->headers.all = vcpu->vmxon->headers.all;
    vcpu->vmcs->headers.bits.shadow_vmcs_indicator = 0;
    return;
}

void init_vcpu(void* info)
{

    struct vcpu_ctx* vcpu;
    uint32_t processor_number;
    uint64_t vm_error;
    struct vmm_ctx** pvmm_context = info;
    struct vmm_ctx* vmm_context = *pvmm_context;

    preempt_disable();

    adjust_control_registers();

    processor_number = raw_smp_processor_id();
    vcpu = vmm_context->vcpu_table[processor_number];

    vcpu->id = processor_number;

    printk(KERN_INFO "init logical vcpu %d\n", processor_number);

    if (__vmxon(vcpu->vmxon_physical)) 
    {
        printk(
            KERN_INFO "Failed to put vcpu %d into vmxon\n", 
            processor_number
        );
        free_vcpu(vcpu);
        vmm_context->vcpu_table[processor_number] = 0;
        return;
    }

    printk(KERN_INFO "vcpu %d is in vmx operation\n", processor_number);

    if (__vmclear(vcpu->vmcs_physical)) 
    {
        printk(
            KERN_INFO "Fail to vmclear on vcpu %d, VMCS is at %llX\n", 
            processor_number, 
            vcpu->vmcs_physical
        );
        free_vcpu(vcpu);
        vmm_context->vcpu_table[processor_number] = 0;
        return;
    }
    
    printk(
        KERN_INFO "vcpu %d: %llX has been vmclear\n", 
        processor_number, 
        vcpu->vmcs_physical
    );

    if (__vmptrld(vcpu->vmcs_physical)) 
    {
        printk(
            KERN_INFO "Fail to vmptrld on vcpu %d, VMCS is at %llX\n", 
            processor_number, 
            vcpu->vmcs_physical
        );
        free_vcpu(vcpu);
        vmm_context->vcpu_table[processor_number] = 0;
        return;
    }

    printk(
        KERN_INFO "vcpu %d: vmptrld to %llX\n", 
        processor_number, 
        vcpu->vmcs_physical
    );

    if (ept_setup_proc(vcpu))
    {
        printk(KERN_ALERT "Failed to setup ept for proc\n");
        return;
    }

    init_vmcs(
        vcpu, 
        (void*)get_rsp(), 
        (void (*)(void))__vmlaunch + 6
    );

    dump_vmcs();

    vm_error = __vmlaunch();

    printk(
        KERN_INFO "At this point we're executing in GUEST MODE for vcpu %d\n",
        processor_number
    );

    preempt_enable();
    return;
}

int exit_vcpu(struct vcpu_ctx* vcpu)
{
    __vmclear(vcpu->vmcs_physical);
    __vmxoff();
    return 0;
}

int init_vcpu_vmxon(struct vcpu_ctx* vcpu)
{
    union __vmx_basic_msr_t vmx_basic = { 0 };
    struct _vmcs* vmxon;

    if (!vcpu) 
    {
        printk(KERN_INFO "vmxon could not be initialized: vcpu in null\n");
        return -1;
    }

    // starting to build vcpu, allocate it
    vmx_basic.control = native_read_msr(MSR_IA32_VMX_BASIC);

    vcpu->vmxon = (struct _vmcs*)__get_free_page(GFP_ATOMIC | GFP_KERNEL);
    memset(vcpu->vmxon, 0, PAGE_SIZE);

    // then set pa address and the headers
    vcpu->vmxon_physical = __pa(vcpu->vmxon);
    vmxon = vcpu->vmxon;
    vmxon->headers.all = vmx_basic.bits.vmcs_revision_identifier;

    printk(
        KERN_INFO "VMXON for vcpu initialized:\n\t-> VA: %llX\n\t"
        "-> PA: %llX\n\t-> REV: %X\n",
        (uint64_t)vcpu->vmxon,
        vcpu->vmxon_physical,
        vcpu->vmxon->headers.all
    );

    return 0;
}

void free_vcpu(struct vcpu_ctx* vcpu)
{
    free_page((uint64_t)vcpu->vmcs);
    free_page((uint64_t)vcpu->vmxon);
    free_page((uint64_t)vcpu->msr_bitmap);
    kfree(vcpu->vmm_stack);
    kfree(vcpu);
}

