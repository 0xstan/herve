#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <asm/special_insns.h>
#include <linux/device.h>
#include <linux/fs.h>

#include "exit.h"
#include "utils.h"
#include "vmcs.h"
#include "vmxasm.h"
#include "vcpu.h"

void adjust_rip()
{
    vmwrite(
        GUEST_RIP, 
        vmread(GUEST_RIP) + vmread(VM_EXIT_INSTRUCTION_LEN)
    );
    return;
}

uint64_t retrieve_guest_flags()
{
    return  vmread(GUEST_RFLAGS);
}

void leave_vmx_mode(
    struct vcpu_ctx* cpu, 
    struct vmexit_guest_registers* guest_registers
)
{
    adjust_rip();

    cpu->guest_rip = vmread(GUEST_RIP);
    cpu->guest_rsp = vmread(GUEST_RSP);
    cpu->guest_flags = retrieve_guest_flags();

    exit_vcpu(cpu);

    guest_registers->rax = (uint64_t)cpu;
    detach(guest_registers);
}

int ept_handle_violation(
    struct vcpu_ctx* cpu, 
    struct vmexit_guest_registers* guest_registers)
{
    union ept_violation_exit_qualification violation;
    uint64_t guest_phys_addr;
    uint64_t guest_addr;
    violation.flags = vmread(EXIT_QUALIFICATION);
    guest_phys_addr = vmread(GUEST_PHYSICAL_ADDRESS);
    guest_addr = vmread(GUEST_RIP);
    
    printk(
        KERN_INFO 
        "Ept Violation = %llx at %llx RIP = %16llx\n", 
        violation.flags, guest_phys_addr, guest_addr);
    return VMEXIT_UNHANDLED;
}

uint64_t guest_cpuid(
    struct vcpu_ctx* cpu, 
    struct vmexit_guest_registers* guest_registers)
{

    uint32_t res[4];

    if (
        guest_registers->rax == 0xdeadbeef && 
        guest_registers->rcx == 0xcafebabe)
    {
        printk(
            KERN_INFO 
            "[CPU %4llx] "
            "Exiting vmx mode\n",
            cpu->id
        );
        leave_vmx_mode(cpu, guest_registers);
        // THIS NEVER RETURN !!!! 
    }

    printk(
        KERN_INFO 
        "[CPU %4llx] "
        "Intercepting CPUID with leaf=%08llx and subleaf=%08llx\n",
        cpu->id,
        guest_registers->rax & 0xFFFFFFFF,
        guest_registers->rcx & 0xFFFFFFFF
    );

    __mycpuid(
        res, 
        guest_registers->rax & 0xFFFFFFFF, 
        guest_registers->rcx & 0xFFFFFFFF
    );

    guest_registers->rax = res[0];
    guest_registers->rbx = res[1];
    guest_registers->rcx = res[2];
    guest_registers->rdx = res[3];

    return VMEXIT_HANDLED;
}

int vmexit_handler(
    struct vcpu_ctx* cpu, 
    struct vmexit_guest_registers* guest_registers)
{
    union __vmx_exit_reason_field_t vmexit_reason;
    uint64_t vmexit_status = VMEXIT_UNHANDLED;

    vmexit_reason.flags = vmread(VM_EXIT_REASON);

    switch (vmexit_reason.flags) 
    {
        case vmexit_ept_violation:
            vmexit_status = ept_handle_violation(cpu, guest_registers);
            break;
        case vmexit_cpuid:
            vmexit_status = guest_cpuid(cpu, guest_registers);
            break;
        default:
            vmexit_status = VMEXIT_UNHANDLED;
            break;
    }

    if (vmexit_status == VMEXIT_UNHANDLED) 
    {
        printk(KERN_INFO "UNHANDLED %llx\n", vmexit_reason.flags);
        __asm__ __volatile__("ud2");
    }

    adjust_rip();

    return vmexit_status;
}
