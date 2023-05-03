#include <checker.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/time64.h>
#include <linux/timer.h>

/*
 * This module shows how to create a simple subdirectory in sysfs called
 * /sys/kernel/kobject-example  In that directory, 3 files are created:
 * "foo", "baz", and "bar".  If an integer is written to these files, it can be
 * later read out of it.
 */

static int sum = 0;
static int baz;
static int bar;

/*
 * The "foo" file where a static variable is read from and written to.
 */
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

/* Sysfs attributes cannot be world-writable. */
static struct kobj_attribute sum_attribute =
    __ATTR(my_sys, 0664, sum_show, sum_store);

/*
 * More complex function where we determine which variable is being accessed by
 * looking at the attribute for the "baz" and "bar" files.
 */
static ssize_t b_show(struct kobject *kobj, struct kobj_attribute *attr,
                      char *buf) {
    int var;

    if (strcmp(attr->attr.name, "baz") == 0)
        var = baz;
    else
        var = bar;
    return sprintf(buf, "%d\n", var);
}

static ssize_t b_store(struct kobject *kobj, struct kobj_attribute *attr,
                       const char *buf, size_t count) {
    int var, ret;

    ret = kstrtoint(buf, 10, &var);
    if (ret < 0) return ret;

    if (strcmp(attr->attr.name, "baz") == 0)
        baz = var;
    else
        bar = var;
    return count;
}

static struct kobj_attribute baz_attribute = __ATTR(baz, 0664, b_show, b_store);
static struct kobj_attribute bar_attribute = __ATTR(bar, 0664, b_show, b_store);

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *attrs[] = {
    &sum_attribute.attr, &baz_attribute.attr, &bar_attribute.attr,
    NULL, /* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory.  If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *example_kobj;

static int a = 0;
module_param(a, int, 0);

static int b = 0;
module_param(b, int, 0);

// #define NSEC_PER_MSEC 1000000L

static unsigned long delays[100];
static int num = 0;  // = sizeof(delays)/sizeof(delays[0]);
module_param_array(delays, long, &num, S_IRUGO | S_IWUSR);
// int length = module_param_array_named (module_param_array﻿)
// int length = 10;
static int n = 0;
static struct hrtimer hr_timer;

/*void my_timer_callback( unsigned long data )
{
    if (n > length)
        return;
    hrtimer_forward_now(current_hrtimer_ptr, ktime_set(0, (delays[n] *
NSEC_PER_MSEC))); n++;
    //check_timer();
}*/

enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer) {
    // printk( "my_hrtimer_callback called (%ld).\n", jiffies );
    printk(KERN_INFO "111 n = %d num = %d", n, num);
    if (n > num) return HRTIMER_NORESTART;

    ktime_t ktime;
    ktime = ktime_set(0, delays[n] * NSEC_PER_MSEC);
    n++;
    check_timer();
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

    return HRTIMER_RESTART;
}

static int __init example_init(void) {
    printk(KERN_INFO "0000000000000000000");
    ktime_t ktime;
    unsigned long delay_in_ms = 200L;
    printk(KERN_INFO "1111111 = %d", delays[n]);
    ktime = ktime_set(0, delays[0] * NSEC_PER_MSEC);
    hrtimer_init(&hr_timer, CLOCK_REALTIME, HRTIMER_MODE_REL);
    hr_timer.function = &my_hrtimer_callback;
    check_timer();
    n++;
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

    return 0;
}

static void __exit example_exit(void) {
    int ret;
    ret = hrtimer_cancel(&hr_timer);
    if (ret) printk("The timer was still in use...\n");
    printk("HR Timer module uninstalling\n");

    return;
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
