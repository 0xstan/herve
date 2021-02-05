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

#include "vmcs.h"
#include "vmx.h"
#include "vmxasm.h"
#include "arch.h"
#include "utils.h"

void vmcs_setup_controls(struct vcpu_ctx* vcpu)
{
    union __vmx_entry_control_t entry_controls;
    union __vmx_exit_control_t exit_controls;
    union __vmx_pinbased_control_msr_t pinbased_controls;
    union __vmx_primary_processor_based_control_t primary_controls;
    union __vmx_secondary_processor_based_control_t secondary_controls;
    entry_controls.control = native_read_msr(MSR_IA32_VMX_ENTRY_CTLS);
    entry_controls.bits.ia32e_mode_guest = 1;

    exit_controls.control = native_read_msr(MSR_IA32_VMX_EXIT_CTLS);
    exit_controls.bits.host_address_space_size = 1;

    pinbased_controls.control = 
        native_read_msr(MSR_IA32_VMX_TRUE_PINBASED_CTLS);

    primary_controls.control = 
        native_read_msr(MSR_IA32_VMX_TRUE_PROCBASED_CTLS);
    primary_controls.bits.active_secondary_controls = 1;
    primary_controls.bits.use_msr_bitmaps = 1;

    secondary_controls.control = 0;
    secondary_controls.bits.enable_invpcid = 1;
    secondary_controls.bits.enable_xsave_xrstor= 1;
    secondary_controls.bits.enable_rdtscp = 1;
    secondary_controls.bits.enable_ept = 1;


    vmx_adjust_entry_controls(&entry_controls);
    vmx_adjust_exit_controls(&exit_controls);
    vmx_adjust_pinbased_controls( &pinbased_controls);
    vmx_adjust_primary_processor_based_controls( &primary_controls );
    vmx_adjust_secondary_processor_based_controls( &secondary_controls );

    vmwrite(PIN_BASED_VM_EXEC_CONTROL, pinbased_controls.control);
    vmwrite(CPU_BASED_VM_EXEC_CONTROL, primary_controls.control);
    vmwrite(SECONDARY_VM_EXEC_CONTROL, secondary_controls.control );
    vmwrite(VM_EXIT_CONTROLS, exit_controls.control );
    vmwrite(VM_ENTRY_CONTROLS, entry_controls.control );
    vmwrite(EXCEPTION_BITMAP, 0);
    vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
    vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);
    vmwrite(CR3_TARGET_COUNT, 0);
    vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
    vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
    vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
    vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);
    vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, 0);

    vmwrite(MSR_BITMAP, __pa(vcpu->msr_bitmap));

    vmwrite(CR0_GUEST_HOST_MASK, 0);
    vmwrite(CR4_GUEST_HOST_MASK, 0);
    vmwrite(CR0_READ_SHADOW, __myread_cr0());
    vmwrite(CR4_READ_SHADOW, __myread_cr4());

    return;
}

void vmcs_setup_guest(void* guest_rsp, void (*guest_rip)(void))
{
    int processor_number;
    struct __pseudo_descriptor_64_t gdtr;
    struct __pseudo_descriptor_64_t idtr;
    uint16_t ldtr = __read_ldtr();
    uint16_t tr = __read_tr();

    vmwrite(GUEST_CR0, __myread_cr0());
    vmwrite(GUEST_CR3, __myread_cr3());
    vmwrite(GUEST_CR4, __myread_cr4());
    vmwrite(GUEST_DR7, 0x400); // KVM does this
    vmwrite(GUEST_RFLAGS, get_rflags()); // KVM does this

    vmwrite(GUEST_RIP, (uint64_t)guest_rip);
    vmwrite(GUEST_RSP, (uint64_t)guest_rsp);

    vmwrite(GUEST_CS_SELECTOR, __read_cs());
    vmwrite(GUEST_SS_SELECTOR, __read_ss());
    vmwrite(GUEST_DS_SELECTOR, __read_ds());
    vmwrite(GUEST_ES_SELECTOR, __read_es());
    vmwrite(GUEST_FS_SELECTOR, __read_fs());
    vmwrite(GUEST_GS_SELECTOR, __read_gs());
    vmwrite(GUEST_TR_SELECTOR, __read_tr());
    vmwrite(GUEST_LDTR_SELECTOR, ldtr);

    vmwrite(GUEST_CS_LIMIT, __segment_limit(__read_cs()));
    vmwrite(GUEST_SS_LIMIT, __segment_limit(__read_ss()));
    vmwrite(GUEST_DS_LIMIT, __segment_limit(__read_ds()));
    vmwrite(GUEST_ES_LIMIT, __segment_limit(__read_es()));
    vmwrite(GUEST_FS_LIMIT, __segment_limit(__read_fs()));
    vmwrite(GUEST_GS_LIMIT, __segment_limit(__read_gs()));
    vmwrite(GUEST_LDTR_LIMIT, __segment_limit(ldtr));
    vmwrite(GUEST_TR_LIMIT, __segment_limit(tr));

    vmwrite(GUEST_CS_AR_BYTES, read_segment_access_rights(__read_cs()));
    vmwrite(GUEST_SS_AR_BYTES, read_segment_access_rights(__read_ss()));
    vmwrite(GUEST_DS_AR_BYTES, read_segment_access_rights(__read_ds()));
    vmwrite(GUEST_ES_AR_BYTES, read_segment_access_rights(__read_es()));
    vmwrite(GUEST_FS_AR_BYTES, read_segment_access_rights(__read_fs()));
    vmwrite(GUEST_GS_AR_BYTES, read_segment_access_rights(__read_gs()));
    vmwrite(GUEST_LDTR_AR_BYTES, read_segment_access_rights(ldtr));
    vmwrite(GUEST_TR_AR_BYTES, read_segment_access_rights(tr));

    vmwrite(GUEST_FS_BASE, native_read_msr(MSR_FS_BASE));
    vmwrite(GUEST_GS_BASE, native_read_msr(MSR_GS_BASE));

    __mystore_gdt(&gdtr); 
    __mystore_idt(&idtr); 

    vmwrite(GUEST_GDTR_LIMIT, gdtr.limit);
    vmwrite(GUEST_IDTR_LIMIT, idtr.limit);
    vmwrite(GUEST_GDTR_BASE, gdtr.base_address);
    vmwrite(GUEST_IDTR_BASE, idtr.base_address);

    vmwrite(GUEST_LDTR_BASE, __segmentbase(gdtr.base_address, ldtr));

    processor_number = raw_smp_processor_id();
    vmwrite(GUEST_TR_BASE, __segmentbase(gdtr.base_address, tr));

    vmwrite(GUEST_IA32_DEBUGCTL, 0);
    vmwrite(GUEST_SYSENTER_CS, native_read_msr(MSR_IA32_SYSENTER_CS));
    vmwrite(GUEST_SYSENTER_EIP, native_read_msr(MSR_IA32_SYSENTER_EIP));
    vmwrite(GUEST_SYSENTER_ESP, native_read_msr(MSR_IA32_SYSENTER_ESP));

    vmwrite(GUEST_ACTIVITY_STATE, 0);
    vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);
    vmwrite(GUEST_PENDING_DBG_EXCEPTIONS, 0);
    vmwrite(VMCS_LINK_POINTER, 0xFFFFFFFFFFFFFFFF);


    return;
}

void vmcs_setup_host(
    struct vcpu_ctx* vcpu, 
    void* host_rsp, 
    void (*host_rip)(void)
)
{
    uint16_t selector_mask = 0b111;
    struct __pseudo_descriptor_64_t gdtr;
    struct __pseudo_descriptor_64_t idtr;
    int processor_number;
    uint16_t tr = __read_tr();
    __mystore_gdt(&gdtr); 
    __mystore_idt(&idtr); 


    vmwrite(HOST_CR0, __myread_cr0());
    vmwrite(HOST_CR3, vcpu->vmm_context->host_pgd);
    vmwrite(HOST_CR4, __myread_cr4());
    
    __vmwrite(HOST_RSP, (uint64_t)host_rsp);
    __vmwrite(HOST_RIP, (uint64_t)host_rip);

    vmwrite(HOST_CS_SELECTOR, __read_cs() & ~selector_mask);
    vmwrite(HOST_SS_SELECTOR, __read_ss() & ~selector_mask);
    vmwrite(HOST_DS_SELECTOR, __read_ds() & ~selector_mask);
    vmwrite(HOST_ES_SELECTOR, __read_es() & ~selector_mask);
    vmwrite(HOST_FS_SELECTOR, __read_fs() & ~selector_mask);
    vmwrite(HOST_GS_SELECTOR, __read_gs() & ~selector_mask);
    vmwrite(HOST_TR_SELECTOR, __read_tr() & ~selector_mask);
    vmwrite(HOST_FS_BASE, native_read_msr(MSR_FS_BASE));
    vmwrite(HOST_GS_BASE, native_read_msr(MSR_GS_BASE));
    vmwrite(HOST_GDTR_BASE, gdtr.base_address);
    vmwrite(HOST_IDTR_BASE, idtr.base_address);

    processor_number = raw_smp_processor_id();
    vmwrite(HOST_TR_BASE, __segmentbase(gdtr.base_address, tr));
    
    vmwrite(HOST_IA32_SYSENTER_CS, native_read_msr(MSR_IA32_SYSENTER_CS));

    vmwrite(
        HOST_IA32_SYSENTER_EIP, 
        native_read_msr(MSR_IA32_SYSENTER_EIP)
    );
    vmwrite(
        HOST_IA32_SYSENTER_ESP, 
        native_read_msr(MSR_IA32_SYSENTER_ESP)
    );

    vmwrite(EPT_POINTER, vcpu->eptp.flags);
}

int init_vmcs(struct vcpu_ctx* vcpu, void* guest_rsp, void (*guest_rip)(void))
{
    vmcs_setup_controls(vcpu);
    vmcs_setup_guest(guest_rsp, guest_rip);
    vmcs_setup_host(vcpu, &vcpu->vmm_stack->cpu, vm_entrypoint);
    return 0;
}
