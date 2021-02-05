#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <asm/special_insns.h>
#include <linux/device.h>
#include <linux/fs.h>

#include "vmx.h"
#include "vmm.h"
#include "vcpu.h"
#include "arch.h"
#include "vmxasm.h"


int enable_vmx_operations()
{
    union __cpuid_t cpuid = { 0 };
    union __cr4_t cr4 = { 0 };
    union __ia32_feature_control_msr_t feature_msr = { 0 };
    printk(KERN_INFO "Starting herve!\n");

    __mycpuid(cpuid.cpu_info, 1, 0);

    printk(
        KERN_INFO 
        "VMX extension: %d", 
        cpuid.feature_ecx.virtual_machine_extensions
    );

    printk(KERN_INFO "Setting cr4.vmx_enable to 1");
    cr4.control = __myread_cr4();
    cr4.bits.vmx_enable = 1;
    native_write_cr4(cr4.control); 

    feature_msr.control = native_read_msr(MSR_IA32_FEATURE_CONTROL);
    if (!feature_msr.bits.lock) {
        printk(
            KERN_INFO 
            "Setting MSR_IA32_FEATURE_CONTROL lock bit and vmxon_outside_smx"
            "bit to 1"
        );
        feature_msr.bits.lock = 1;
        feature_msr.bits.vmxon_outside_smx = 1;
        native_write_msr(
            MSR_IA32_FEATURE_CONTROL, 
            feature_msr.control & 0xFFFFFFFF, 
            (feature_msr.control >> 32) & 0xFFFFFFFF
        );
        return 0;
    }
    return -1;
}

void end_vmx(void){
    __send_stop_cpuid();
    return;
}

