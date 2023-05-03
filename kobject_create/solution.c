/*
 * Разработать модуль ядра solution, который создает kobject с именем
 * /sys/kernel/my_kobject/my_sys. В этом объекте должно отображаться актуальное
 * число операций чтения, адресованных к данному kobject.
 */

#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>

static int my_sys = 0;

static ssize_t my_sys_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf) {
    my_sys++;
    return sprintf(buf, "%d\n", my_sys);
}

static ssize_t my_sys_store(struct kobject *kobj, struct kobj_attribute *attr,
                            const char *buf, size_t count) {
    int ret;

    ret = kstrtoint(buf, 10, &my_sys);
    if (ret < 0) return ret;

    return count;
}

static struct kobj_attribute my_sys_attribute =
    __ATTR(my_sys, 0664, my_sys_show, my_sys_store);
static struct attribute *attrs[] = {
    &my_sys_attribute.attr,
    NULL,
};
static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *example_kobj;

static int __init example_init(void) {
    int retval;

    example_kobj = kobject_create_and_add("my_kobject", kernel_kobj);
    if (!example_kobj) return -ENOMEM;

    retval = sysfs_create_group(example_kobj, &attr_group);
    if (retval) kobject_put(example_kobj);

    return retval;
}

static void __exit example_exit(void) { kobject_put(example_kobj); }

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
