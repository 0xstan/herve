#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/workqueue.h>
#include <asm/special_insns.h>
#include <linux/device.h>
#include <linux/fs.h>

#include "entry.h"
#include "vmm.h"

int opt_enable_hypervisor_no;

struct kobj_attribute do_enable =
    __ATTR(enable, 0664, opt_enable_hypervisor_show, opt_enable_hypervisor_set);
struct attribute* attrs[] = {&do_enable.attr, NULL};
struct attribute_group attr_group = {.attrs = attrs};

struct kobject* herve_kobj;

static 
ssize_t opt_enable_hypervisor_show(
    struct kobject* kobj,
    struct kobj_attribute* attr,
    char* buf) 
{
    return sprintf(buf, "%d\n", opt_enable_hypervisor_no);
}

static 
ssize_t opt_enable_hypervisor_set(
    struct kobject* kobj,
    struct kobj_attribute* attr,
    const char* buf, size_t count) 
{
    int ret;
    if ((ret = kstrtoint(buf, 10, &opt_enable_hypervisor_no)) < 0) {
        return ret;
    }

    switch (opt_enable_hypervisor_no) {
        case 1:
            ret = herve_init_hv();
            break;
        case 0:
            ret = herve_exit_hv();
            break;
        default:
            printk(KERN_ALERT "invalid value\n");
            ret = -EINVAL;
    }

    if (ret < 0) {
        printk(KERN_ALERT "failed to execute specified action\n");
    }
    return count;
}

static 
int __init herve_init(void) 
{
    int ret;
    if (!(herve_kobj = kobject_create_and_add("herve", kernel_kobj))) 
    {
        printk(KERN_ALERT "failed to create kernel object\n");
        return -ENOMEM;
    }

    if ((ret = sysfs_create_group(herve_kobj, &attr_group)))
    {
        kobject_put(herve_kobj);
    }
    printk(KERN_INFO "Starting herve!\n");

    return ret;
}

static void __exit herve_exit(void) {
    kobject_put(herve_kobj);
    printk(KERN_INFO "Stopping herve!\n");
}

module_init(herve_init);
module_exit(herve_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("stan");
MODULE_DESCRIPTION("That goddamn herve");
MODULE_VERSION("1");
