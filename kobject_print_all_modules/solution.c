/*
 * Разработать модуль ядра solution, который обращается к связному списку
 * структуры struct module из заголовочного файла linux/module.h и выводит имена
 * всех модулей в kobject с именем /sys/kernel/my_kobject/my_sys. Имена модулей
 * должны выводиться в алфавитном порядке, каждое на новой строке.
 */

#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <linux/sysfs.h>

static int my_sys = 0;

static ssize_t my_sys_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf) {
    struct list_head *module_head;

    struct list_head *listptr;
    struct module *entry;

    char buf_tmp[4096];
    char str_mas[100][100];
    int i = 0;

    list_for_each(listptr, THIS_MODULE->list.prev) {
        entry = list_entry(listptr, struct module, list);
        strcpy(str_mas[i], entry->name);
        i++;
    }

    int k, l;
    char temp[100];
    for (k = 0; k < i - 1; k++) {
        for (l = 0; l < i - k - 1; l++) {
            if (strcmp(str_mas[l], str_mas[l + 1]) == 1) {
                strcpy(temp, str_mas[l]);
                strcpy(str_mas[l], str_mas[l + 1]);
                strcpy(str_mas[l + 1], temp);
            }
        }
    }

    int j;
    for (j = 0; j < i; j++) {
        char tmp_str[100];
        sprintf(tmp_str, "%s\n", str_mas[j]);
        strcat(buf_tmp, tmp_str);
    }
    buf_tmp[strlen(buf_tmp) - 1] = '\0';

    return sprintf(buf, "%s", buf_tmp);
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
