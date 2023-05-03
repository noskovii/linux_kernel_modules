/*
 * Разработать модуль ядра solution, который вызывает функцию из модуля ядра
 * checker, определенную прототипом:
 *     void call_me(const char* message);
 * С аргументом "Hello from my module!". Прототип задается в файле checker.h.
 */

#include <checker.h>
#include <linux/module.h>

int init_module(void) {
    printk(KERN_INFO "Hello, loading");
    call_me("Hello from my module!");
}

void cleanup_module(void) { printk(KERN_INFO "Leaving"); }

MODULE_LICENSE("GPL");
