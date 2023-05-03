#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <linux/sysfs.h>

/*
 * This module shows how to create a simple subdirectory in sysfs called
 * /sys/kernel/kobject-example  In that directory, 3 files are created:
 * "foo", "baz", and "bar".  If an integer is written to these files, it can be
 * later read out of it.
 */

static int my_sys = 0;
static int baz;
static int bar;

/*
 * The "foo" file where a static variable is read from and written to.
 */
static ssize_t my_sys_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf) {
    struct list_head *module_head;
    // module_head = THIS_MODULE->list.prev;

    struct list_head *listptr;
    struct module *entry;

    char buf_tmp[4096];
    char str_mas[100][100];
    int i = 0;

    list_for_each(listptr, THIS_MODULE->list.prev) {
        entry = list_entry(listptr, struct module, list);
        // printk(KERN_INFO "i = %d, str = %s", i, entry->name);
        strcpy(str_mas[i], entry->name);
        i++;
    }

    printk(KERN_INFO "i = %d", i);

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
        printk(KERN_INFO "j = %d str = %s", j, tmp_str);
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

/* Sysfs attributes cannot be world-writable. */
static struct kobj_attribute my_sys_attribute =
    __ATTR(my_sys, 0664, my_sys_show, my_sys_store);

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
    &my_sys_attribute.attr, &baz_attribute.attr, &bar_attribute.attr,
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

static int __init example_init(void) {
    int retval;

    /*
     * Create a simple kobject with the name of "kobject_example",
     * located under /sys/kernel/
     *
     * As this is a simple directory, no uevent will be sent to
     * userspace.  That is why this function should not be used for
     * any type of dynamic kobjects, where the name and number are
     * not known ahead of time.
     */
    example_kobj = kobject_create_and_add("my_kobject", kernel_kobj);
    if (!example_kobj) return -ENOMEM;

    /* Create the files associated with this kobject */
    retval = sysfs_create_group(example_kobj, &attr_group);
    if (retval) kobject_put(example_kobj);

    return retval;
}

static void __exit example_exit(void) { kobject_put(example_kobj); }

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
