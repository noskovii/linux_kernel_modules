/*
 * Разработать модуль ядра solution, который реализует файловый интерфейс
 * драйвера символьного устройства. В качестве устройства необходимо
 * использовать node, задаваемый через строковый параметр модуля node_name.
 * Драйвер должен сам создать node по заданному имени в /dev и получить для него
 * major number. Полученное значение необходимо предоставлять пользователям при
 * попытках чтения из устройства.
 *
 * Пример загрузки модуля:
 *     sudo insmod solution.ko node_name="my_super_cool_but_random_node_name"
 *
 * Результаты работы драйвера должны быть доступны при чтении с символьного
 * устройства в следующем формате MAJOR\n, где MAJOR - это major number,
 * динамически полученный модулем.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static dev_t dev;
static unsigned int count = 1;
static int my_major = 700, my_minor = 0;
static struct cdev *my_cdev;

#define MYDEV_NAME "mychrdev"
#define KBUF_SIZE (size_t)((10) * PAGE_SIZE)

static struct class *my_class;

#define FIXLEN 1000
static char module_name[FIXLEN] = "";

module_param_string(node_name, module_name, sizeof(module_name), 0);

static int mychrdev_open(struct inode *inode, struct file *file) {
    static int counter = 0;

    char *kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);

    file->private_data = kbuf;

    printk(KERN_INFO "Opening device %s:\n\n", MYDEV_NAME);
    counter++;

    printk(KERN_INFO, "Counter: %d\n", counter);
    printk(KERN_INFO, "Module refcounter: %d\n", module_refcount(THIS_MODULE));

    return 0;
}

static int mychrdev_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Releasing device %s:\n\n", MYDEV_NAME);

    char *kbuf = file->private_data;

    printk(KERN_INFO "Free buffer");
    if (kbuf) kfree(kbuf);
    kbuf = NULL;
    file->private_data = NULL;

    return 0;
}

static bool first_req = true;
static ssize_t mychrdev_read(struct file *file, char __user *buf, size_t lbuf,
                             loff_t *ppos) {
    if (!first_req) return 0;

    char *tmp = kmalloc(KBUF_SIZE, GFP_KERNEL);
    int size_of_message = sprintf(tmp, "%d\n", my_major);
    int nbytes = lbuf - copy_to_user(buf, tmp, size_of_message);
    first_req = false;

    return nbytes;
}

static ssize_t mychrdev_write(struct file *file, const char __user *buf,
                              size_t lbuf, loff_t *ppos) {
    char *kbuf = file->private_data;

    int nbytes = lbuf - copy_from_user(kbuf + *ppos, buf, lbuf);
    *ppos += nbytes;

    printk(KERN_INFO "Write device %s nbytes = %d, ppos = %d:\n\n", MYDEV_NAME,
           nbytes, (int)*ppos);
    return nbytes;
}

static loff_t mychrdev_lseek(struct file *file, loff_t offset, int orig) {
    loff_t testpos;

    switch (orig) {
        case SEEK_SET:
            testpos = offset;
            break;
        case SEEK_CUR:
            testpos = file->f_pos + offset;
            break;
        case SEEK_END:
            testpos = KBUF_SIZE + offset;
            break;
        default:
            return -EINVAL;
    }

    testpos = testpos < KBUF_SIZE ? testpos : KBUF_SIZE;
    testpos = testpos >= 0 ? testpos : 0;

    file->f_pos = testpos;
    printk(KERN_INFO "Seeking to %ld position", (long)testpos);
    return testpos;
}

static const struct file_operations mycdev_fops = {.owner = THIS_MODULE,
                                                   .read = mychrdev_read,
                                                   .write = mychrdev_write,
                                                   .open = mychrdev_open,
                                                   .release = mychrdev_release,
                                                   .llseek = mychrdev_lseek};

static int __init init_chrdev(void) {
    printk(KERN_INFO "Hello, loading");

    dev = MKDEV(my_major, my_minor);
    register_chrdev_region(dev, count, MYDEV_NAME);
    my_cdev = cdev_alloc();

    cdev_init(my_cdev, &mycdev_fops);
    cdev_add(my_cdev, dev, count);

    my_class = class_create(THIS_MODULE, "my_class");
    device_create(my_class, NULL, dev, "%s", module_name);
    printk(KERN_INFO "Create device class %s", MYDEV_NAME);

    return 0;
}

static void __exit cleanup_chrdev(void) {
    printk(KERN_INFO "Leaving");

    device_destroy(my_class, dev);
    class_destroy(my_class);

    if (my_cdev) cdev_del(my_cdev);
    unregister_chrdev_region(dev, count);
}

module_init(init_chrdev);
module_exit(cleanup_chrdev);

MODULE_LICENSE("GPL");
