#include <checker.h>
#include <linux/module.h>

int init_module(void) {
    CHECKER_MACRO;

    int i;
    for (i = 0; i < 11; i++) {
        short mas[i];
        int j;
        int sum = 0;

        for (j = 0; j < i; j++) {
            mas[j] = j;
            sum += j;
        }

        int res = array_sum(mas, i);
        printk("kernel_mooc res = %d", res);

        char str[512];
        int size = generate_output(res, mas, i, str);
        str[size] = '\0';

        if (res == sum) {
            printk(KERN_INFO "%s", str);
            printk("kernel_mooc INFO = *%d* %s", sum, str);
        } else {
            printk(KERN_ERR "%s", str);
            printk("kernel_mooc ERR = *%d* %s", sum, str);
        }
    }
}

void exit_module(void) { CHECKER_MACRO; }

module_exit(exit_module);

MODULE_LICENSE("GPL");
