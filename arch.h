#ifndef ARCH_H
#define ARCH_H
union __cr_fixed_t
{
    struct
    {
        uint32_t low;
        uint32_t high;
    } split;
    uint64_t all;
};

union __cr0_t
{
    uint64_t control;
};

union __cr4_t
{
	uint64_t control;
	struct
	{
		uint64_t vme : 1;									 // bit 0
		uint64_t pvi : 1;									 // bit 1
		uint64_t time_stamp_disable : 1;					 // bit 2
		uint64_t debug_extensions : 1;						 // bit 3
		uint64_t page_size_extension : 1;					 // bit 4
		uint64_t physical_address_extension : 1;			 // bit 5
		uint64_t machine_check_enable : 1;					 // bit 6
		uint64_t page_global_enable : 1;					 // bit 7
		uint64_t perf_counter_enable : 1;					 // bit 8
		uint64_t os_fxsave_support : 1;						 // bit 9
		uint64_t os_xmm_exception_support : 1;				 // bit 10
		uint64_t usermode_execution_prevention : 1;			 // bit 11
		uint64_t reserved_0 : 1;							 // bit 12
		uint64_t vmx_enable : 1;							 // bit 13
		uint64_t smx_enable : 1;							 // bit 14
		uint64_t reserved_1 : 1;							 // bit 15
		uint64_t fs_gs_enable : 1;							 // bit 16
		uint64_t pcide : 1;									 // bit 17
		uint64_t os_xsave : 1;								 // bit 18
		uint64_t reserved_2 : 1;							 // bit 19
		uint64_t smep : 1;									 // bit 20
		uint64_t smap : 1;									 // bit 21
		uint64_t protection_key_enable : 1;					 // bit 22
	} bits;
};

struct __segment_descriptor_64_t
{
  uint16_t segment_limit_low;
  uint16_t base_low;
  union
  {
    struct
    {
      uint32_t base_middle                                           : 8;
      uint32_t type                                                  : 4;
      uint32_t descriptor_type                                       : 1;
      uint32_t dpl                                           : 2;
      uint32_t present                                               : 1;
      uint32_t segment_limit_high                                    : 4;
      uint32_t system                                                : 1;
      uint32_t long_mode                                             : 1;
      uint32_t default_big                                           : 1;
      uint32_t granularity                                           : 1;
      uint32_t base_high                                         : 8;
    } bits;
    uint32_t flags;
  } ;
  uint32_t base_upper;
  uint32_t reserved;
};

struct __segment_descriptor_32_t
{
  uint16_t segment_limit_low;
  uint16_t base_low;
  union
  {
    struct
    {
      uint32_t base_middle                                           : 8;
      uint32_t type                                                  : 4;
      uint32_t descriptor_type                                       : 1;
      uint32_t dpl                                           : 2;
      uint32_t present                                               : 1;
      uint32_t segment_limit_high                                    : 4;
      uint32_t system                                                : 1;
      uint32_t long_mode                                             : 1;
      uint32_t default_big                                           : 1;
      uint32_t granularity                                           : 1;
      uint32_t base_high                                         : 8;
    } bits;
    uint32_t flags;
  };
};

union __segment_selector_t
{
  struct
  {
    uint16_t rpl                                   : 2;
    uint16_t table                                 : 1;
    uint16_t index                                 : 13;
  } bits;
  uint16_t flags;
};

union __segment_access_rights_t
{
  struct
  {
    uint32_t type                                                    : 4;
    uint32_t descriptor_type                                         : 1;
    uint32_t dpl                                                     : 2;
    uint32_t present                                                 : 1;
    uint32_t reserved0                                               : 4;
    uint32_t available                                               : 1;
    uint32_t long_mode                                               : 1;
    uint32_t default_big                                             : 1;
    uint32_t granularity                                             : 1;
    uint32_t unusable                                                : 1;
    uint32_t reserved1                                               : 15;
  } bits;
  uint32_t flags;
};

union __rflags_t{
    uint64_t flags;
    struct {
        uint64_t cf: 1;
        uint64_t reserved0: 1;
        uint64_t pf: 1;
        uint64_t reserved1: 1;
        uint64_t af: 1;
        uint64_t reserved2: 1;
        uint64_t zf: 1;
        uint64_t sf: 1;
        uint64_t tf: 1;
        uint64_t if_: 1;
        uint64_t df: 1;
        uint64_t of: 1;
        uint64_t iopl: 1;
        uint64_t nt: 1;
        uint64_t reserved3: 1;
        uint64_t rf: 1;
        uint64_t vm: 1;
        uint64_t ac: 1;
        uint64_t vif: 1;
        uint64_t vip: 1;
        uint64_t id: 1;
        uint64_t upper: 42;
    } bits;
};

typedef struct {
	uint64_t ptr;
	uint64_t gpa;
} invept_t;

typedef struct {
	uint64_t vpid : 16;
	uint64_t rsvd : 48;
	uint64_t gva;
} invvpid_t;

struct __pseudo_descriptor_64_t
{
  uint16_t limit;
  uint64_t base_address;
} __attribute__((packed));

typedef union __packed {
	uint64_t all;
	struct {
		uint64_t limit_low : 16;
		uint64_t base_low : 16;
		uint64_t base_mid : 8;
		uint64_t type : 4;
		uint64_t system : 1;
		uint64_t dpl : 2;
		uint64_t present : 1;
		uint64_t limit_high : 4;
		uint64_t avl : 1;
		uint64_t l : 1;
		uint64_t db : 1;
		uint64_t gran : 1;
		uint64_t base_high : 8;
	};
} segmentdesc_t;

typedef struct __packed {
	segmentdesc_t d32;
	uint32_t base_upper32;
	uint32_t reserved;
} segmentdesc64_t;

#endif
