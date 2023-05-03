#include <checker.h>
#include <linux/module.h>
#include <linux/slab.h>

void *mem_1;
int *mem_2;
struct device *mem_3;

int init_module(void) {
    int size_1 = get_void_size();
    mem_1 = kmalloc(sizeof(void) * size_1, GFP_KERNEL);
    submit_void_ptr(mem_1);

    int size_2 = get_int_array_size();
    mem_2 = kmalloc(sizeof(int) * size_2, GFP_KERNEL);
    submit_int_array_ptr(mem_2);

    mem_3 = kmalloc(sizeof(struct device), GFP_KERNEL);
    submit_struct_ptr(mem_3);
}

void exit_module(void) {
    checker_kfree(mem_1);
    checker_kfree(mem_2);
    checker_kfree(mem_3);
}

module_exit(exit_module);

MODULE_LICENSE("GPL");
