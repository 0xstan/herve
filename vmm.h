#ifndef VMM_H
#define VMM_H
#define VMM_STACK_SIZE 2* PAGE_SIZE

struct __mtrr_range_descriptor
{
    uint64_t physical_base_address;
    uint64_t physical_end_address;
    uint8_t memory_type;
};

struct vmm_ctx {
    unsigned long processor_count;
    unsigned long host_pgd;
    struct __mtrr_range_descriptor memory_ranges[9]; 
    uint64_t enabled_memory_ranges;
    struct vcpu_ctx** vcpu_table;
};

void allocate_vmm_context(void);
int vmm_init(void);
void free_objects(struct vmm_ctx* vmm_context);
void free_vmm_context(struct vmm_ctx* vmm_context);
int herve_init_hv(void);
int herve_exit_hv(void);
#endif
