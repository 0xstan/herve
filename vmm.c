#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
#include <linux/mm.h>
#else
#include <linux/sched/mm.h>
#endif

#include <linux/workqueue.h>
#include <asm/special_insns.h>
#include <linux/device.h>
#include <linux/fs.h>
#include "vmm.h"
#include "vmx.h"
#include "vcpu.h"
#include "ept.h"


struct vmm_ctx* vmm_context;
struct mm_struct *mm = NULL;

void allocate_vmm_context()
{
    vmm_context = 
        (struct vmm_ctx*) 
        kzalloc(
            sizeof( struct vmm_ctx ), 
            GFP_KERNEL | GFP_ATOMIC
        );

    mm = current->active_mm;
    atomic_inc(&mm->mm_count);
    vmm_context->host_pgd = __pa(mm->pgd);

    vmm_context->enabled_memory_ranges = 0;

    if (vmm_context == NULL) 
    {
        printk(KERN_INFO "Vmm_context could not be allocated\n");
        return;
    }


    printk(KERN_INFO "Number of active cpus: %d\n", num_active_cpus());
    vmm_context->processor_count = num_active_cpus();
    vmm_context->vcpu_table = 
        (struct vcpu_ctx**) 
        kzalloc(
            sizeof( struct vcpu_ctx*) * vmm_context->processor_count, 
            GFP_KERNEL | GFP_ATOMIC
        );
    printk(KERN_INFO "vmm_context allocated at %llX\n", (uint64_t)vmm_context);
    printk(
        KERN_INFO 
        "vcpu_table allocated at %llX\n", 
        (uint64_t)vmm_context->vcpu_table
    );
}


int vmm_init()
{
    int i;
    int cpu;
    struct vcpu_ctx* vcpu;
    
    printk(KERN_INFO "Allocating VMM context\n");
    allocate_vmm_context();

    if (ept_init(vmm_context))
    {
        printk(KERN_INFO "Failed initializing EPT\n");
        free_vmm_context(vmm_context);
        return -1;
    }

    for (i = 0; i < vmm_context->processor_count; i++) 
    {
        printk(KERN_INFO "Allocating vcpu %d\n", i);
        vmm_context->vcpu_table[i] = allocate_vcpu();
        
        vcpu = vmm_context->vcpu_table[i];
        vcpu->vmm_context = vmm_context;

        if (init_vcpu_vmxon(vcpu)) 
        {
            printk(KERN_INFO "fail to init vmxon on vcpu %d\n", i);
            free_vcpu(vcpu);
            vmm_context->vcpu_table[i] = 0;
        }

        init_vcpu_structs(vcpu);
        printk(KERN_INFO "vcpu %d structs have been initialized\n", i);
    }

    for_each_online_cpu(cpu)
        smp_call_function_single(
            cpu, 
            (smp_call_func_t)init_vcpu, 
            (void*)&vmm_context, 
            1
        );

    return 0;
}

void free_objects(struct vmm_ctx* vmm_context )
{
    int i;
    for (i = 0 ; i < vmm_context->processor_count; i++) 
    {
        free_vcpu(vmm_context->vcpu_table[i]);
    }
    free_vmm_context(vmm_context);
}

void free_vmm_context(struct vmm_ctx* vmm_context)
{
   kfree(vmm_context->vcpu_table);
   kfree(vmm_context);
   atomic_dec(&mm->mm_count); 
   // First I was using mmdrop but it leads to triple fault 
   // on 5.X after vmxoff. Probably because the CR3 register is still used
   // somewhere ?? mmdrop should not delete this if it's still used I guess. 
}

int herve_init_hv()
{
    enable_vmx_operations();
    vmm_init();
    return 0;
}

int herve_exit_hv()
{
    int cpu;
    preempt_disable();
    for_each_online_cpu(cpu)
        smp_call_function_single(cpu, (smp_call_func_t)end_vmx, NULL, 1);
    preempt_enable();
    free_objects(vmm_context);
    printk(KERN_INFO "Stopping hypervisor\n");
    return 0;
}
