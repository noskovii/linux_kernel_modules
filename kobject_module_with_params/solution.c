/*
 * Разработать модуль ядра solution, который принимает на вход следующие
 * параметры:
 *     - a типа int
 *     - b типа int
 *     - c массив из 5ти элементов типа int
 * Модуль считает сумму a+b+c[@], создает kobject с именем
 * /sys/kernel/my_kobject/my_sys и выводит в него результат вычисления.
 */

#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>

static int sum = 0;

static ssize_t sum_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
    return sprintf(buf, "%d\n", sum);
}

static ssize_t sum_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count) {
    int ret;

    ret = kstrtoint(buf, 10, &sum);
    if (ret < 0) return ret;

    return count;
}

static struct kobj_attribute sum_attribute =
    __ATTR(my_sys, 0664, sum_show, sum_store);

static struct attribute *attrs[] = {
    &sum_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *example_kobj;

static int a = 0;
module_param(a, int, 0);

static int b = 0;
module_param(b, int, 0);

static int c[] = {0, 0, 0, 0, 0};
static int num = sizeof(c) / sizeof(c[0]);
module_param_array(c, int, &num, S_IRUGO | S_IWUSR);

static int __init example_init(void) {
    int retval;

    sum += a;
    sum += b;
    int i;
    for (i = 0; i < num; i++) {
        sum += c[i];
    }

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
