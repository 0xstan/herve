#ifndef VMX_H
#define VMX_H
union __ia32_efer_t
{
  uint64_t control;
  struct
  {
    uint64_t syscall_enable : 1;
    uint64_t reserved_0 : 7;
    uint64_t long_mode_enable : 1;
    uint64_t reserved_1 : 1;
    uint64_t long_mode_active : 1;
    uint64_t execute_disable : 1;
    uint64_t reserved_2 : 52;
  } bits;
};

union __ia32_feature_control_msr_t
{
  uint64_t control;
  struct
  {
    uint64_t lock : 1;
    uint64_t vmxon_inside_smx : 1;
    uint64_t vmxon_outside_smx : 1;
    uint64_t reserved_0 : 5;
    uint64_t senter_local : 6;
    uint64_t senter_global : 1;
    uint64_t reserved_1 : 1;
    uint64_t sgx_launch_control_enable : 1;
    uint64_t sgx_global_enable : 1;
    uint64_t reserved_2 : 1;
    uint64_t lmce : 1;
    uint64_t system_reserved : 42;
  } bits;
};

union __vmx_misc_msr_t
{
  uint64_t control;
  struct
  {
    uint64_t vmx_preemption_tsc_rate : 5;
    uint64_t store_lma_in_vmentry_control : 1;
    uint64_t activate_state_bitmap : 3;
    uint64_t reserved_0 : 5;
    uint64_t pt_in_vmx : 1;
    uint64_t rdmsr_in_smm : 1;
    uint64_t cr3_target_value_count : 9;
    uint64_t max_msr_vmexit : 3;
    uint64_t allow_smi_blocking : 1;
    uint64_t vmwrite_to_any : 1;
    uint64_t interrupt_mod : 1;
    uint64_t reserved_1 : 1;
    uint64_t mseg_revision_identifier : 32;
  } bits;
};

union __vmx_basic_msr_t
{
  uint64_t control;
  struct
  {
    uint64_t vmcs_revision_identifier : 31;
    uint64_t always_0 : 1;
    uint64_t vmxon_region_size : 13;
    uint64_t reserved_1 : 3;
    uint64_t vmxon_physical_address_width : 1;
    uint64_t dual_monitor_smi : 1;
    uint64_t memory_type : 4;
    uint64_t io_instruction_reporting : 1;
    uint64_t true_controls : 1;
  } bits;
};

union __vmx_pinbased_control_msr_t
{
  uint64_t control;
  struct
  {
    uint64_t external_interrupt_exiting : 1;
    uint64_t reserved_0 : 2;
    uint64_t nmi_exiting : 1;
    uint64_t reserved_1 : 1;
    uint64_t virtual_nmis : 1;
    uint64_t vmx_preemption_timer : 1;
    uint64_t process_posted_interrupts : 1;
  } bits;
};

union __vmx_entry_control_t
{
  uint64_t control;
  struct
  {
    uint64_t reserved_0 : 2;
    uint64_t load_dbg_controls : 1;
    uint64_t reserved_1 : 6;
    uint64_t ia32e_mode_guest : 1;
    uint64_t entry_to_smm : 1;
    uint64_t deactivate_dual_monitor_treament : 1;
    uint64_t reserved_3 : 1;
    uint64_t load_ia32_perf_global_control : 1;
    uint64_t load_ia32_pat : 1;
    uint64_t load_ia32_efer : 1;
    uint64_t load_ia32_bndcfgs : 1;
    uint64_t conceal_vmx_from_pt : 1;
  } bits;
};

union __vmx_true_control_settings_t
{
    uint64_t control;
    struct
    {
        uint32_t allowed_0_settings;
        uint32_t  allowed_1_settings;
    };
};

union __cpuid_t
{
    struct
    {
        uint32_t cpu_info[4];
    };

    struct
    {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    } registers;

    struct
    {
        union
        {
            uint32_t flags;

            struct
            {
                uint32_t stepping_id : 4;
                uint32_t model : 4;
                uint32_t family_id : 4;
                uint32_t processor_type : 2;
                uint32_t reserved1 : 2;
                uint32_t extended_model_id : 4;
                uint32_t extended_family_id : 8;
                uint32_t reserved2 : 4;
            };
        } version_information;

        union
        {
            uint32_t flags;

            struct
            {
                uint32_t brand_index : 8;
                uint32_t clflush_line_size : 8;
                uint32_t max_addressable_ids : 8;
                uint32_t initial_apic_id : 8;
            } additional_information;
        };

        union
        {
            uint32_t flags;

            struct
            {
                uint32_t streaming_simd_extensions_3 : 1;
                uint32_t pclmulqdq_instruction : 1;
                uint32_t ds_area_64bit_layout : 1;
                uint32_t monitor_mwait_instruction : 1;
                uint32_t cpl_qualified_debug_store : 1;
                uint32_t virtual_machine_extensions : 1;
                uint32_t safer_mode_extensions : 1;
                uint32_t enhanced_intel_speedstep_technology : 1;
                uint32_t thermal_monitor_2 : 1;
                uint32_t supplemental_streaming_simd_extensions_3 : 1;
                uint32_t l1_context_id : 1;
                uint32_t silicon_debug : 1;
                uint32_t fma_extensions : 1;
                uint32_t cmpxchg16b_instruction : 1;
                uint32_t xtpr_update_control : 1;
                uint32_t perfmon_and_debug_capability : 1;
                uint32_t reserved1 : 1;
                uint32_t process_context_identifiers : 1;
                uint32_t direct_cache_access : 1;
                uint32_t sse41_support : 1;
                uint32_t sse42_support : 1;
                uint32_t x2apic_support : 1;
                uint32_t movbe_instruction : 1;
                uint32_t popcnt_instruction : 1;
                uint32_t tsc_deadline : 1;
                uint32_t aesni_instruction_extensions : 1;
                uint32_t xsave_xrstor_instruction : 1;
                uint32_t osx_save : 1;
                uint32_t avx_support : 1;
                uint32_t half_precision_conversion_instructions : 1;
                uint32_t rdrand_instruction : 1;
                uint32_t reserved2 : 1;
            };
        } feature_ecx;

        union
        {
            uint32_t flags;

            struct
            {
                uint32_t floating_point_unit_on_chip : 1;
                uint32_t virtual_8086_mode_enhancements : 1;
                uint32_t debugging_extensions : 1;
                uint32_t page_size_extension : 1;
                uint32_t timestamp_counter : 1;
                uint32_t rdmsr_wrmsr_instructions : 1;
                uint32_t physical_address_extension : 1;
                uint32_t machine_check_exception : 1;
                uint32_t cmpxchg8b : 1;
                uint32_t apic_on_chip : 1;
                uint32_t reserved : 1;
                uint32_t sysenter_sysexit_instructions : 1;
                uint32_t memory_type_range_registers : 1;
                uint32_t page_global_bit : 1;
                uint32_t machine_check_architecture : 1;
                uint32_t conditional_move_instructions : 1;
                uint32_t page_attribute_table : 1;
                uint32_t page_size_extension_36bit : 1;
                uint32_t processor_serial_number : 1;
                uint32_t clflush : 1;
                uint32_t reserved2 : 1;
                uint32_t debug_store : 1;
                uint32_t thermal_control_msrs_for_acpi : 1;
                uint32_t mmx_support : 1;
                uint32_t fxsave_fxrstor_instructions : 1;
                uint32_t sse_support : 1;
                uint32_t sse2_support : 1;
                uint32_t self_snoop : 1;
                uint32_t hyper_threading_technology : 1;
                uint32_t thermal_monitor : 1;
                uint32_t reserved3 : 1;
                uint32_t pending_break_enable : 1;
            };
        } feature_edx;
    };
};

union __vmx_primary_processor_based_control_t
{
    uint64_t control;
    struct
    {
        uint64_t reserved_0 : 2;
        uint64_t interrupt_window_exiting : 1;
        uint64_t use_tsc_offsetting : 1;
        uint64_t reserved_1 : 3;
        uint64_t hlt_exiting : 1;
        uint64_t reserved_2 : 1;
        uint64_t invldpg_exiting : 1;
        uint64_t mwait_exiting : 1;
        uint64_t rdpmc_exiting : 1;
        uint64_t rdtsc_exiting : 1;
        uint64_t reserved_3 : 2;
        uint64_t cr3_load_exiting : 1;
        uint64_t cr3_store_exiting : 1;
        uint64_t reserved_4 : 2;
        uint64_t cr8_load_exiting : 1;
        uint64_t cr8_store_exiting : 1;
        uint64_t use_tpr_shadow : 1;
        uint64_t nmi_window_exiting : 1;
        uint64_t mov_dr_exiting : 1;
        uint64_t unconditional_io_exiting : 1;
        uint64_t use_io_bitmaps : 1;
        uint64_t reserved_5 : 1;
        uint64_t monitor_trap_flag : 1;
        uint64_t use_msr_bitmaps : 1;
        uint64_t monitor_exiting : 1;
        uint64_t pause_exiting : 1;
        uint64_t active_secondary_controls : 1;
    } bits;
};

union __vmx_secondary_processor_based_control_t
{
    uint64_t control;
    struct
    {
        uint64_t virtualize_apic_accesses : 1;
        uint64_t enable_ept : 1;
        uint64_t descriptor_table_exiting : 1;
        uint64_t enable_rdtscp : 1;
        uint64_t virtualize_x2apic : 1;
        uint64_t enable_vpid : 1;
        uint64_t wbinvd_exiting : 1;
        uint64_t unrestricted_guest : 1;
        uint64_t apic_register_virtualization : 1;
        uint64_t virtual_interrupt_delivery : 1;
        uint64_t pause_loop_exiting : 1;
        uint64_t rdrand_exiting : 1;
        uint64_t enable_invpcid : 1;
        uint64_t enable_vmfunc : 1;
        uint64_t vmcs_shadowing : 1;
        uint64_t enable_encls_exiting : 1;
        uint64_t rdseed_exiting : 1;
        uint64_t enable_pml : 1;
        uint64_t use_virtualization_exception : 1;
        uint64_t conceal_vmx_from_pt : 1;
        uint64_t enable_xsave_xrstor : 1;
        uint64_t reserved_0 : 1;
        uint64_t mode_based_execute_control_ept : 1;
        uint64_t reserved_1 : 2;
        uint64_t use_tsc_scaling : 1;
    } bits;
};

union __vmx_exit_control_t
{
    uint64_t control;
    struct
    {
        uint64_t reserved_0 : 2;
        uint64_t save_dbg_controls : 1;
        uint64_t reserved_1 : 6;
        uint64_t host_address_space_size : 1;
        uint64_t reserved_2 : 2;
        uint64_t load_ia32_perf_global_control : 1;
        uint64_t reserved_3 : 2;
        uint64_t ack_interrupt_on_exit : 1;
        uint64_t reserved_4 : 2;
        uint64_t save_ia32_pat : 1;
        uint64_t load_ia32_pat : 1;
        uint64_t save_ia32_efer : 1;
        uint64_t load_ia32_efer : 1;
        uint64_t save_vmx_preemption_timer_value : 1;
        uint64_t clear_ia32_bndcfgs : 1;
        uint64_t conceal_vmx_from_pt : 1;
    } bits;
};

int enable_vmx_operations(void);
void end_vmx(void);
#endif
