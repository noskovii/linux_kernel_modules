/*
 * Разработать модуль ядра solution, который успешно компилируется и загружается
 * через insmod.
 */

#include <linux/module.h>

int init_module(void) { printk(KERN_INFO "Hello, loading"); }

void cleanup_module(void) { printk(KERN_INFO "Leaving"); }

MODULE_LICENSE("GPL");
