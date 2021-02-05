#ifndef EXIT_H

#include "vcpu.h"

#define EXIT_H
#define VMEXIT_HANDLED 0
#define VMEXIT_UNHANDLED 1
#define VMEXIT_QUIT 2

struct vmexit_guest_registers
{
    unsigned __int128 xmm[6]; 
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rbp;
} ;

union __vmx_exit_reason_field_t
{
    uint64_t flags;
    struct
    {
        uint64_t basic_exit_reason : 16;
        uint64_t must_be_zero_1 : 11;
        uint64_t was_in_enclave_mode : 1;
        uint64_t pending_mtf_exit : 1;
        uint64_t exit_from_vmx_root : 1;
        uint64_t must_be_zero_2 : 1;
        uint64_t vm_entry_failure : 1;
    } bits;
};

enum __vmexit_reason_e
{
    vmexit_nmi = 0,
    vmexit_ext_int,
    vmexit_triple_fault,
    vmexit_init_signal,
    vmexit_sipi,
    vmexit_smi,
    vmexit_other_smi,
    vmexit_interrupt_window,
    vmexit_nmi_window,
    vmexit_task_switch,
    vmexit_cpuid,
    vmexit_getsec,
    vmexit_hlt,
    vmexit_invd,
    vmexit_invlpg,
    vmexit_rdpmc,
    vmexit_rdtsc,
    vmexit_rsm,
    vmexit_vmcall,
    vmexit_vmclear,
    vmexit_vmlaunch,
    vmexit_vmptrld,
    vmexit_vmptrst,
    vmexit_vmread,
    vmexit_vmresume,
    vmexit_vmwrite,
    vmexit_vmxoff,
    vmexit_vmxon,
    vmexit_control_register_access,
    vmexit_mov_dr,
    vmexit_io_instruction,
    vmexit_rdmsr,
    vmexit_wrmsr,
    vmexit_vmentry_failure_due_to_guest_state,
    vmexit_vmentry_failure_due_to_msr_loading,
    vmexit_mwait = 36,
    vmexit_monitor_trap_flag,
    vmexit_monitor = 39,
    vmexit_pause,
    vmexit_vmentry_failure_due_to_machine_check_event,
    vmexit_tpr_below_threshold = 43,
    vmexit_apic_access,
    vmexit_virtualized_eoi,
    vmexit_access_to_gdtr_or_idtr,
    vmexit_access_to_ldtr_or_tr,
    vmexit_ept_violation,
    vmexit_ept_misconfiguration,
    vmexit_invept,
    vmexit_rdtscp,
    vmexit_vmx_preemption_timer_expired,
    vmexit_invvpid,
    vmexit_wbinvd,
    vmexit_xsetbv,
    vmexit_apic_write,
    vmexit_rdrand,
    vmexit_invpcid,
    vmexit_vmfunc,
    vmexit_encls,
    vmexit_rdseed,
    vmexit_pml_full,
    vmexit_xsaves,
    vmexit_xrstors,
};

void adjust_rip(void);
uint64_t retrieve_guest_flags(void);
void leave_vmx_mode(
    struct vcpu_ctx* cpu, 
    struct vmexit_guest_registers* guest_registers
);
int ept_handle_violation(
    struct vcpu_ctx*, 
    struct vmexit_guest_registers*);
uint64_t guest_cpuid(
    struct vcpu_ctx*, struct vmexit_guest_registers* guest_registers
);
int vmexit_handler(
    struct vcpu_ctx*, struct vmexit_guest_registers* guest_registers
);
#endif
