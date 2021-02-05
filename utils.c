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


#include "utils.h"
#include "vmxasm.h"
#include "vmcs.h"
#include "vcpu.h"

uint64_t vmread(uint64_t field)
{
    uint64_t res; 
    __vmread(field, &res);
    return res;
}

void vmwrite(uint64_t field, uint64_t value)
{
    __vmwrite(field, value);
}

inline segmentdesc_t *segment_desc(uintptr_t gdt, uint16_t sel)
{
	return (segmentdesc_t *)(gdt + (sel & ~3));
}

uintptr_t segment_desc_base(segmentdesc_t *desc)
{
	uintptr_t base = 
        desc->base_high << 24
        | desc->base_mid << 16
        | desc->base_low;

	if (!desc->system)
		base |= (uintptr_t)(((segmentdesc64_t *)desc)->base_upper32) << 32;

	return base;
}

uintptr_t __segmentbase(uintptr_t gdt, uint16_t sel)
{
	if (!sel)
		return 0;

	/* If it's an LDT segment, load the LDT  */
	if (sel & 4) 
    {
		segmentdesc_t *ldt = segment_desc(gdt, __read_ldtr());
		uintptr_t ldt_base = segment_desc_base(ldt);
		return segment_desc_base(segment_desc(ldt_base, sel));
	}

	return segment_desc_base(segment_desc(gdt, sel));
}


uint64_t vmx_adjust_cv(uint64_t capability_msr, uint64_t value)
{
    union __vmx_true_control_settings_t cap;
    unsigned int actual;

    cap.control = native_read_msr( capability_msr );
    actual = value;

    actual |= cap.allowed_0_settings;
    actual &= cap.allowed_1_settings;
    return actual;
}

void vmx_adjust_entry_controls(
    union __vmx_entry_control_t *entry_controls
)
{
    uint64_t capability_msr;
    union __vmx_basic_msr_t basic;

    basic.control = native_read_msr( MSR_IA32_VMX_BASIC );
    capability_msr = 
        (basic.bits.true_controls != 0) ? 
        MSR_IA32_VMX_TRUE_ENTRY_CTLS : MSR_IA32_VMX_ENTRY_CTLS;

    entry_controls->control = 
        vmx_adjust_cv( capability_msr, entry_controls->control );
}

void vmx_adjust_exit_controls(union __vmx_exit_control_t *exit_controls)
{
    unsigned int capability_msr;
    union __vmx_basic_msr_t basic;
    basic.control = native_read_msr( MSR_IA32_VMX_BASIC );
    capability_msr = 
        (basic.bits.true_controls != 0) ? 
        MSR_IA32_VMX_TRUE_EXIT_CTLS : MSR_IA32_VMX_EXIT_CTLS;
    exit_controls->control = 
        vmx_adjust_cv( capability_msr, exit_controls->control );
}

void vmx_adjust_pinbased_controls(
    union __vmx_pinbased_control_msr_t *pinbased_controls
)
{
    unsigned int capability_msr;
    union __vmx_basic_msr_t basic;
    basic.control = native_read_msr( MSR_IA32_VMX_BASIC );
    capability_msr = (basic.bits.true_controls != 0) ? 
        MSR_IA32_VMX_TRUE_PINBASED_CTLS : MSR_IA32_VMX_PINBASED_CTLS;
    pinbased_controls->control = 
        vmx_adjust_cv( capability_msr, pinbased_controls->control );
}

void vmx_adjust_primary_processor_based_controls(
    union __vmx_primary_processor_based_control_t *ppbc)
{
    unsigned int capability_msr;
    union __vmx_basic_msr_t basic;
    basic.control = native_read_msr( MSR_IA32_VMX_BASIC );
    capability_msr = (basic.bits.true_controls != 0) ? 
        MSR_IA32_VMX_TRUE_PROCBASED_CTLS : MSR_IA32_VMX_PROCBASED_CTLS;
    ppbc->control = vmx_adjust_cv( capability_msr, ppbc->control );
}

void vmx_adjust_secondary_processor_based_controls(
    union __vmx_secondary_processor_based_control_t *spbc)
{
    union __vmx_basic_msr_t basic;
    basic.control = native_read_msr( MSR_IA32_VMX_BASIC );
    if (basic.bits.true_controls != 0) 
    {
        spbc->control = 
            vmx_adjust_cv(MSR_IA32_VMX_PROCBASED_CTLS2, spbc->control );
    }
}

void adjust_control_registers()
{
    union __cr4_t cr4 = { 0 };
    union __cr0_t cr0 = { 0 };
    union __cr_fixed_t cr_fixed = { 0 };

    cr_fixed.all = native_read_msr(MSR_IA32_VMX_CR0_FIXED0);
    cr0.control = __myread_cr0();
    cr0.control |= cr_fixed.split.low;
    cr_fixed.all = native_read_msr(MSR_IA32_VMX_CR0_FIXED1);
    cr0.control &= cr_fixed.split.low; 
    native_write_cr0(cr0.control);

    cr_fixed.all = native_read_msr(MSR_IA32_VMX_CR4_FIXED0);
    cr4.control = __myread_cr4();
    cr4.control |= cr_fixed.split.low;
    cr_fixed.all = native_read_msr(MSR_IA32_VMX_CR4_FIXED1);
    cr4.control &= cr_fixed.split.low; 
    native_write_cr4(cr4.control);
}

uint32_t read_segment_access_rights(uint16_t segment_selector)
{
    union __segment_selector_t selector;
    union __segment_access_rights_t vmx_access_rights;

    selector.flags = segment_selector;

    if(selector.bits.table == 0
    && selector.bits.index == 0)
    {
        vmx_access_rights.flags = 0;
        vmx_access_rights.bits.unusable = 1;
        return vmx_access_rights.flags;
    }

    vmx_access_rights.flags = (__load_ar(segment_selector) >> 8);
    vmx_access_rights.bits.unusable = 0;
    vmx_access_rights.bits.reserved0 = 0;
    vmx_access_rights.bits.reserved1 = 0;

    return vmx_access_rights.flags;
}

void dump_guest_state(struct vmexit_guest_registers* guest_registers)
{
    uint64_t rip, rsp, rflags, vm_exit;
    vm_exit = vmread(VM_EXIT_REASON);
    rip = vmread(GUEST_RIP);
    rsp = vmread(GUEST_RSP);
    rflags = retrieve_guest_flags();
    printk(KERN_INFO "========================\n");
    printk(KERN_INFO "CR3: %016llx\n", vmread(GUEST_CR3));
    printk(
        KERN_INFO "RAX: %016llx  RBX: %016llx  RCX: %016llx\n", 
        guest_registers->rax, guest_registers->rbx, guest_registers->rcx
    );
    printk(
        KERN_INFO "RDX: %016llx  RDI: %016llx  RSI: %016llx\n", 
        guest_registers->rdx, guest_registers->rdi, guest_registers->rsi
    );
    printk(
        KERN_INFO "R8:  %016llx  R9:  %016llx  R10: %016llx\n", 
        guest_registers->r8, guest_registers->r9, guest_registers->r10
    );
    printk(
        KERN_INFO "R11: %016llx  R12: %016llx  R13: %016llx\n", 
        guest_registers->r11, guest_registers->r12, guest_registers->r13
    );
    printk(
        KERN_INFO "R14: %016llx  R15: %016llx  RBP: %016llx\n", 
        guest_registers->r14, guest_registers->r15, guest_registers->rbp
    );
    printk(
        KERN_INFO "RSP: %016llx  RIP: %016llx  FLG: %016llx\n", 
        rsp, rip, rflags
    );
    printk(KERN_INFO "EXT: %016llx\n", vm_exit);
    printk(KERN_INFO "========================\n");
    return;
}

void vmx_dump_sel(char *name, uint32_t sel)
{
	printk(
        KERN_INFO "%s sel=0x%016llx, attr=0x%016llx\nlimit=0x%016llx, base=0x%016llx\n",
       name, vmread(sel),
       vmread(sel + GUEST_ES_AR_BYTES - GUEST_ES_SELECTOR),
       vmread(sel + GUEST_ES_LIMIT - GUEST_ES_SELECTOR),
       vmread(sel + GUEST_ES_BASE - GUEST_ES_SELECTOR)
   );
}

void vmx_dump_dtsel(char *name, uint32_t limit)
{
	printk(
        KERN_INFO "%s                           limit=0x%016llx,"
        "base=0x%016llx\n",
       name, vmread(limit),
       vmread(limit + GUEST_GDTR_BASE - GUEST_GDTR_LIMIT)
   );
}

void dump_vmcs(void)
{
	u32 vmentry_ctl, vmexit_ctl;
	u32 cpu_based_exec_ctrl, pin_based_exec_ctrl, secondary_exec_control;
	unsigned long cr4;
	u64 efer;
	int i, n;

	vmentry_ctl = vmread(VM_ENTRY_CONTROLS);
	vmexit_ctl = vmread(VM_EXIT_CONTROLS);
	cpu_based_exec_ctrl = vmread(CPU_BASED_VM_EXEC_CONTROL);
	pin_based_exec_ctrl = vmread(PIN_BASED_VM_EXEC_CONTROL);
	cr4 = vmread(GUEST_CR4);
	efer = vmread(GUEST_IA32_EFER);
	secondary_exec_control = 0;
    secondary_exec_control = vmread(SECONDARY_VM_EXEC_CONTROL);

	printk(KERN_INFO "*** Guest State ***\n");
	printk(
        KERN_INFO "CR0: actual=0x%016llx, shadow=0x%016llx, gh_mask=%016llx\n",
	    vmread(GUEST_CR0), vmread(CR0_READ_SHADOW),
	    vmread(CR0_GUEST_HOST_MASK)
    );
	printk(
        KERN_INFO "CR4: actual=0x%016lx, shadow=0x%016llx, gh_mask=%016llx\n",
	    cr4, vmread(CR4_READ_SHADOW), vmread(CR4_GUEST_HOST_MASK)
    );
	printk(KERN_INFO "CR3 = 0x%016llx\n", vmread(GUEST_CR3));
	printk(KERN_INFO "RSP = 0x%016llx  RIP = 0x%016llx\n",
	       vmread(GUEST_RSP), vmread(GUEST_RIP));
	printk(KERN_INFO "RFLAGS=0x%016llx         DR7 = 0x%016llx\n",
	       vmread(GUEST_RFLAGS), vmread(GUEST_DR7));
	printk(KERN_INFO "Sysenter RSP=%016llx CS:RIP=%016llx:%016llx\n",
	       vmread(GUEST_SYSENTER_ESP),
	       vmread(GUEST_SYSENTER_CS), vmread(GUEST_SYSENTER_EIP));
	vmx_dump_sel("CS:  ", GUEST_CS_SELECTOR);
	vmx_dump_sel("DS:  ", GUEST_DS_SELECTOR);
	vmx_dump_sel("SS:  ", GUEST_SS_SELECTOR);
	vmx_dump_sel("ES:  ", GUEST_ES_SELECTOR);
	vmx_dump_sel("FS:  ", GUEST_FS_SELECTOR);
	vmx_dump_sel("GS:  ", GUEST_GS_SELECTOR);
	vmx_dump_dtsel("GDTR:", GUEST_GDTR_LIMIT);
	vmx_dump_sel("LDTR:", GUEST_LDTR_SELECTOR);
	vmx_dump_dtsel("IDTR:", GUEST_IDTR_LIMIT);
	vmx_dump_sel("TR:  ", GUEST_TR_SELECTOR);
	printk(KERN_INFO "DebugCtl = 0x%016llx  DebugExceptions = 0x%016llx\n",
	       vmread(GUEST_IA32_DEBUGCTL),
	       vmread(GUEST_PENDING_DBG_EXCEPTIONS));
	printk(KERN_INFO "Interruptibility = %016llx  ActivityState = %016llx\n",
	       vmread(GUEST_INTERRUPTIBILITY_INFO),
	       vmread(GUEST_ACTIVITY_STATE));
	printk(KERN_INFO "*** Host State ***\n");
	printk(KERN_INFO "RIP = 0x%016llx  RSP = 0x%016llx\n",
	       vmread(HOST_RIP), vmread(HOST_RSP));
	printk(
        KERN_INFO "CS=%016llx SS=%016llx DS=%016llx"
        " ES=%016llx FS=%016llx GS=%016llx TR=%016llx\n",
	    vmread(HOST_CS_SELECTOR), vmread(HOST_SS_SELECTOR),
	    vmread(HOST_DS_SELECTOR), vmread(HOST_ES_SELECTOR),
	    vmread(HOST_FS_SELECTOR), vmread(HOST_GS_SELECTOR),
	    vmread(HOST_TR_SELECTOR)
    );
	printk(KERN_INFO "FSBase=%016llx GSBase=%016llx TRBase=%016llx\n",
	       vmread(HOST_FS_BASE), vmread(HOST_GS_BASE),
	       vmread(HOST_TR_BASE));
	printk(KERN_INFO "GDTBase=%016llx IDTBase=%016llx\n",
	       vmread(HOST_GDTR_BASE), vmread(HOST_IDTR_BASE));
	printk(KERN_INFO "CR0=%016llx CR3=%016llx CR4=%016llx\n",
	       vmread(HOST_CR0), vmread(HOST_CR3),
	       vmread(HOST_CR4));
	printk(KERN_INFO "Sysenter RSP=%016llx CS:RIP=%016llx:%016llx\n",
	       vmread(HOST_IA32_SYSENTER_ESP),
	       vmread(HOST_IA32_SYSENTER_CS),
	       vmread(HOST_IA32_SYSENTER_EIP));

	printk(KERN_INFO "*** Control State ***\n");
	printk(KERN_INFO "PinBased=%016x CPUBased=%016x SecondaryExec=%016x\n",
	       pin_based_exec_ctrl, cpu_based_exec_ctrl, secondary_exec_control);
	printk(
        KERN_INFO "EntryControls=%016x ExitControls=%016x\n", 
        vmentry_ctl, vmexit_ctl
    );
	printk(KERN_INFO "ExceptionBitmap=%016llx"
           " PFECmask=%016llx PFECmatch=%016llx\n",
	       vmread(EXCEPTION_BITMAP),
	       vmread(PAGE_FAULT_ERROR_CODE_MASK),
	       vmread(PAGE_FAULT_ERROR_CODE_MATCH));
	printk(KERN_INFO "VMEntry: intr_info=%016llx "
           "errcode=%016llx ilen=%016llx\n",
	       vmread(VM_ENTRY_INTR_INFO_FIELD),
	       vmread(VM_ENTRY_EXCEPTION_ERROR_CODE),
	       vmread(VM_ENTRY_INSTRUCTION_LEN));
	printk(KERN_INFO "VMExit: intr_info=%016llx errcode=%016llx"
       " ilen=%016llx\n",
	       vmread(VM_EXIT_INTR_INFO),
	       vmread(VM_EXIT_INTR_ERROR_CODE),
	       vmread(VM_EXIT_INSTRUCTION_LEN));
	printk(KERN_INFO "        reason=%016llx qualification=%016llx\n",
	       vmread(VM_EXIT_REASON), vmread(EXIT_QUALIFICATION));
	printk(KERN_INFO "IDTVectoring: info=%016llx errcode=%016llx\n",
	       vmread(IDT_VECTORING_INFO_FIELD),
	       vmread(IDT_VECTORING_ERROR_CODE));
	printk(KERN_INFO "TSC Offset = 0x%016llx\n", vmread(TSC_OFFSET));
	n = vmread(CR3_TARGET_COUNT);
	for (i = 0; i + 1 < n; i += 4)
		printk(KERN_INFO "CR3 target%u=%016llx target%u=%016llx\n",
		       i, vmread(CR3_TARGET_VALUE0 + i * 2),
		       i + 1, vmread(CR3_TARGET_VALUE0 + i * 2 + 2));
	if (i < n)
		printk(KERN_INFO "CR3 target%u=%016llx\n",
		       i, vmread(CR3_TARGET_VALUE0 + i * 2));
    printk(KERN_INFO "Virtual processor ID = 0x%016llx\n",
       vmread(VIRTUAL_PROCESSOR_ID));
}
