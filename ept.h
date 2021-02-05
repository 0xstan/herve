#ifndef EPT_H
#define EPT_H

#include "vmm.h"

#define IA32_MTRR_PHYSBASE0 0x200
#define IA32_MTRR_PHYSMASK0 0x201

#define MEMORY_TYPE_UNCACHEABLE 0x0
#define MEMORY_TYPE_WRITEBACK 0x6

#define PML4_COUNT 512
#define PML3_COUNT 512
#define PML2_COUNT 512

union ept_pointer
{
    uint64_t flags;
    struct 
    {
        uint64_t memory_type: 3;
        uint64_t page_walk_length: 3;
        uint64_t enable_accessed_and_dirty_flags: 1;
        uint64_t reserved1 : 5;
        uint64_t pml4_address: 36;
        uint64_t reserved2 : 16;
    } bits;
};

union ept_violation_exit_qualification
{
    uint64_t flags;
    struct 
    {
        uint64_t read_access: 1;
        uint64_t write_access: 1;
        uint64_t execute_access: 1;
        uint64_t ept_readable: 1;
        uint64_t ept_writable: 1;
        uint64_t ept_executable: 1;
        uint64_t ept_executable_for_user_mode: 1;
        uint64_t valid_guest_linear_address: 1;
        uint64_t caused_by_translation: 1;
        uint64_t user_mode_linear_address: 1;
        uint64_t readable_writable_page: 1;
        uint64_t executable_disable_page: 1;
        uint64_t nmi_unblocking: 1;
        uint64_t reserved1 : 51;
    } bits;
};

union ept_pml4_entry
{
    uint64_t flags;
    struct 
    {
        uint64_t read_access: 1;
        uint64_t write_access: 1;
        uint64_t execute_access: 1;
        uint64_t reserved1 : 5;
        uint64_t accessed: 1;
        uint64_t reserved2 : 1;
        uint64_t user_mode_execute: 1;
        uint64_t reserved3 : 1;
        uint64_t page_frame_number: 36;
        uint64_t reserved4 : 16;
    } bits;
};

union ept_pml3_entry
{
    uint64_t flags;
    struct 
    {
        uint64_t read_access: 1;
        uint64_t write_access: 1;
        uint64_t execute_access: 1;
        uint64_t reserved1 : 5;
        uint64_t accessed: 1;
        uint64_t reserved2 : 1;
        uint64_t user_mode_execute: 1;
        uint64_t reserved3 : 1;
        uint64_t page_frame_number: 36;
        uint64_t reserved4 : 16;
    } bits;
};

union ept_pml2_entry
{
    uint64_t flags;
    struct 
    {
        uint64_t read_access: 1;
        uint64_t write_access: 1;
        uint64_t execute_access: 1;
        uint64_t memory_type : 3;
        uint64_t ignore_pat: 1;
        uint64_t large_page: 1;
        uint64_t accessed: 1;
        uint64_t dirty: 1;
        uint64_t user_mode_execute: 1;
        uint64_t reserved1 : 10;
        uint64_t page_frame_number: 27;
        uint64_t reserved2 : 15;
        uint64_t suppress_ve: 1;
    } bits;
};

union __ia32_vmx_ept_vpid_cap_register
{
    uint64_t flags;
    struct 
    {
        uint64_t execute_only_pages : 1;
        uint64_t reserved1 : 5;
        uint64_t page_walk_length4 : 1;
        uint64_t reserved2 : 1;
        uint64_t memory_type_uncacheable : 1;
        uint64_t reserved3 : 5;
        uint64_t memory_type_writeback : 1;
        uint64_t reserved4 : 1;
        uint64_t pde_2mb_pages: 1;
        uint64_t pdpte_1gb_pages: 1;
        uint64_t reserved5 : 2;
        uint64_t invept: 1;
        uint64_t ept_accessed_and_dirty_flags: 1;
        uint64_t advanced_vmexit_ept_violation_information: 1;
        uint64_t reserved6 : 2;
        uint64_t invept_single_context: 1;
        uint64_t invept_all_contexts: 1;
        uint64_t reserved7 : 5;
        uint64_t invvpid: 1;
        uint64_t reserved8: 7;
        uint64_t invvpid_individual_address : 1;
        uint64_t invvpid_single_context : 1;
        uint64_t invvpid_all_contexts : 1;
        uint64_t invvpid_single_context_retain_globals : 1;
        uint64_t reserved9 : 20;
    } bits;
};

union __ia32_mtrr_def_type_register
{
    uint64_t flags;
    struct 
    {
        uint64_t default_memory_type : 3;
        uint64_t reserved1: 7;
        uint64_t fixed_range_mtrr_enable : 1;
        uint64_t mtrr_enable : 1;
        uint64_t reserved2: 52;
    } bits;
};

union __ia32_mtrr_capabilities_register
{
    uint64_t flags;
    struct 
    {
        uint64_t variable_range_count : 8;
        uint64_t fixed_range_supported : 1;
        uint64_t reserved1: 1;
        uint64_t wc_supported: 1;
        uint64_t smrr_supported: 1;
        uint64_t reserved2: 52;
    } bits;
};

union __ia32_mtrr_physbase_register
{
    uint64_t flags;
    struct 
    {
        uint64_t type : 8;
        uint64_t reserved1: 4;
        uint64_t page_frame_number: 36;
        uint64_t reserved2: 16;
    } bits;
};

union __ia32_mtrr_physmask_register
{
    uint64_t flags;
    struct 
    {
        uint64_t type : 8;
        uint64_t reserved1: 3;
        uint64_t valid: 1;
        uint64_t page_frame_number: 36;
        uint64_t reserved2: 16;
    } bits;
};

struct page_in_list
{
    void* addr;
    struct list_head list;
};

struct ept_page_table
{
    // Points to a 512 * 8 bytes. Each of the 512 entries represent a 512 GB
    // memory portion
    union ept_pml4_entry *ept_pml4;
    // Points to a 512 * 8 bytes. Each of the 512 entries represent a 1 GB
    // memory portion
    union ept_pml3_entry *ept_pml3;
    // Points to a 512 * 512 * 8 bytes. Each of the 512 entries represent a 
    // 2 MB memory portion
    // There's one table for each 512 ept_pml3 entries, so 512 * 512 
    union ept_pml2_entry *ept_pml2[PML2_COUNT];

    struct list_head splited_pages;
    struct list_head hooked_pages;
};


int ept_check_features(void);
int ept_build_mtrr_map(struct vmm_ctx*);
int ept_init(struct vmm_ctx*);
void free_ept_page_table(struct ept_page_table*);
void setup_pml2_entry(
    struct vmm_ctx* vmm_context, 
    union ept_pml2_entry* entry, 
    uint64_t page_frame_number
);
struct ept_page_table* ept_create_identity_page_table
(
    struct vmm_ctx* vmm_context
);
int ept_setup_proc(struct vcpu_ctx*);
#endif
