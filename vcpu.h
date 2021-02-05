#ifndef VCPU_H
#define VCPU_H

#include "vmm.h"
#include "ept.h"

struct vmm_stack
{
      unsigned char limit[VMM_STACK_SIZE - sizeof(struct vcpu_ctx*)];
      struct vcpu_ctx* cpu;
};

struct vcpu_ctx
{
    uint64_t guest_rsp; // OFFSET 0
    uint64_t guest_rip; // OFFSET 8
    uint64_t guest_flags; // OFFSET 16

    uint64_t id;

    struct ept_page_table* ept_pt; // page_table
    union ept_pointer eptp; // ept_pointer

    struct vmm_ctx* vmm_context;

    struct _vmcs *vmcs;
    uint64_t vmcs_physical;
    struct _vmcs *vmxon;
    uint64_t vmxon_physical;

    struct vmm_stack* vmm_stack;
    char* msr_bitmap;
};


struct vcpu_ctx* allocate_vcpu(void);
void init_vcpu_structs(struct vcpu_ctx* vcpu);
void init_vcpu(void* info);
int exit_vcpu(struct vcpu_ctx* vcpu);
int init_vcpu_vmxon(struct vcpu_ctx* vcpu);
void free_vcpu(struct vcpu_ctx* vcpu);
#endif
