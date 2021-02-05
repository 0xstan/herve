#ifndef UTILS_H
#define UTILS_H
#include "arch.h"
#include "vmx.h"
#include "exit.h"

uint64_t vmread(uint64_t field);
void vmwrite(uint64_t field, uint64_t value);
segmentdesc_t *segment_desc(uintptr_t gdt, uint16_t sel);
uintptr_t segment_desc_base(segmentdesc_t *desc);
uintptr_t __segmentbase(uintptr_t gdt, uint16_t sel);
uint64_t vmx_adjust_cv(uint64_t capability_msr, uint64_t value);
void vmx_adjust_entry_controls(
    union __vmx_entry_control_t *entry_controls
);
void vmx_adjust_exit_controls(union __vmx_exit_control_t *exit_controls);
void vmx_adjust_pinbased_controls(
    union __vmx_pinbased_control_msr_t *pinbased_controls
);
void vmx_adjust_primary_processor_based_controls(
    union __vmx_primary_processor_based_control_t *ppbc);
void vmx_adjust_secondary_processor_based_controls(
    union __vmx_secondary_processor_based_control_t *spbc);
void adjust_control_registers(void);
uint32_t read_segment_access_rights(uint16_t segment_selector);
void dump_guest_state(struct vmexit_guest_registers* guest_registers);
void vmx_dump_sel(char *name, uint32_t sel);
void vmx_dump_dtsel(char *name, uint32_t limit);
void dump_vmcs(void);
#endif
