#ifndef ENTRY_H
#define ENTRY_H
static 
ssize_t opt_enable_hypervisor_show(
    struct kobject* kobj,
    struct kobj_attribute* attr,
    char* buf);

static 
ssize_t opt_enable_hypervisor_set(
    struct kobject* kobj,
    struct kobj_attribute* attr,
    const char* buf, size_t count);

static 
int __init herve_init(void); 

static void __exit herve_exit(void);
#endif
