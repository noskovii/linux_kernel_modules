#include <checker.h>
#include <linux/module.h>

int init_module(void) {
    printk(KERN_INFO "Hello, loading");
    call_me("Hello from my module!");
}

void cleanup_module(void) { printk(KERN_INFO "Leaving"); }

MODULE_LICENSE("GPL");
