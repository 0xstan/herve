#ifndef VMXASM_H
#define VMXASM_H

#include "arch.h"
#include "exit.h"

extern void __mystore_gdt(struct __pseudo_descriptor_64_t*);
extern void __mystore_idt(struct __pseudo_descriptor_64_t*);
extern uint64_t __myread_cr0(void);
extern uint64_t __myread_cr3(void);
extern uint64_t __myread_cr4(void);
extern uint16_t __read_ldtr(void);
extern uint16_t __read_tr(void);
extern uint16_t __read_cs(void);
extern uint16_t __read_ss(void);
extern uint16_t __read_ds(void);
extern uint16_t __read_es(void);
extern uint16_t __read_fs(void);
extern uint16_t __read_gs(void);
extern uint64_t __segment_limit(uint32_t);
extern uint64_t __load_ar(uint16_t);
extern uint64_t __vmlaunch(void);
extern uint64_t __vmcall(void);
extern uint64_t __vmread(uint64_t, uint64_t*);
extern uint64_t __vmxon(uint64_t);
extern uint64_t __vmxoff(void);
extern uint64_t __vmclear(uint64_t);
extern uint64_t __vmptrld(uint64_t);
extern uint64_t __vmwrite(uint64_t, uint64_t);
extern uint64_t __mycpuid(uint32_t*, uint32_t, uint32_t);
extern uint64_t get_rsp(void);
extern uint64_t get_rflags(void);
extern uint64_t __send_stop_cpuid(void);
extern uint64_t __bit_scan_forward(uint64_t);
extern void vm_entrypoint(void);
extern void detach(struct vmexit_guest_registers* guest_registers);
#endif
