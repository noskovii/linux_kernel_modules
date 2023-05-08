/*
 * Необходимо создать модуль ядра, реализующий обработчик прерываний для
 * устройства Real Time Clock, использующего IRQ №8. Обработчик прерываний
 * должен вести подсчет прерываний и отображать его в kobject
 * /sys/kernel/my_kobject/my_sys.
 *
 * Для самостоятельного тестирования рекомендуется использовать команду
 *     sudo bash -c ' echo +20 > /sys/class/rtc/rtc0/wakealarm '
 * которая осуществляет установку таймера для RTC и файл /proc/interrupts для
 * подсчета числа прерываний.
 */

#include <asm/irq.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#define IRQ_LINE 8
static int irq = IRQ_LINE, my_dev_id, irq_counter = 0;

static int my_sys = 0;
static int count = 0;

static irqreturn_t my_interrupt(int irq, void *dev_id) { count++; }

static ssize_t my_sys_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf) {
    return sprintf(buf, "%d", count);
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

    request_irq(irq, my_interrupt, IRQF_SHARED, "my_interrupt", &my_dev_id);

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
