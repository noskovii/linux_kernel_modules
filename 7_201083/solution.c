#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>  // PAGE_SIZE
#include <linux/uaccess.h>

static dev_t first;
static unsigned int count = 1;
static int my_major = 240, my_minor = 0;
static struct cdev *my_cdev;

#define MYDEV_NAME "mychrdev"
#define KBUF_SIZE (size_t)((10) * PAGE_SIZE)

static int sum = 0, sum_old = 0, data_sum, data_sum_old = 0;

static int mychrdev_open(struct inode *inode, struct file *file) {
    sum++;

    char *kbuf = kmalloc(
        KBUF_SIZE,
        GFP_KERNEL);  // выделяем для каждого приложения/открытия свой буфер
    file->private_data = kbuf;  // file передается из приложения

    printk(KERN_INFO "Opening device %s:\n\n", MYDEV_NAME);
    printk(KERN_INFO, "module refcounter: %d\n", module_refcount(THIS_MODULE));

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

static ssize_t mychrdev_read(struct file *file, char __user *buf, size_t lbuf,
                             loff_t *ppos) {
    if (sum_old == sum && data_sum_old == data_sum) return 0;

    char *tmp = kmalloc(KBUF_SIZE, GFP_KERNEL);
    int num = sprintf(tmp, "%d %d\n", sum, data_sum);
    int nbytes =
        lbuf -
        copy_to_user(buf, tmp,
                     num);  // copy_to_user вернет 0 если все скопировано, если
                            // не 0 - то покажет сколько не скопировано

    sum_old = sum;
    data_sum_old = data_sum;

    return nbytes;
}

static ssize_t mychrdev_write(struct file *file, const char __user *buf,
                              size_t lbuf, loff_t *ppos) {
    char *kbuf = file->private_data;

    int nbytes = lbuf - copy_from_user(kbuf + *ppos, buf, lbuf);
    *ppos += nbytes;

    data_sum += nbytes;

    printk(KERN_INFO "Write device %s nbytes = %d, ppos = %d:\n\n", MYDEV_NAME,
           nbytes, (int)*ppos);
    return nbytes;
}

static const struct file_operations mycdev_fops = {.owner = THIS_MODULE,
                                                   .read = mychrdev_read,
                                                   .write = mychrdev_write,
                                                   .open = mychrdev_open,
                                                   .release = mychrdev_release};

static int __init init_chrdev(void) {
    printk(KERN_INFO "Hello, loading");

    first = MKDEV(my_major, my_minor);  // создание ноды на файловой системе
    register_chrdev_region(
        first, count,
        MYDEV_NAME);  // регистрируем регион (множество идентификаторов) для
                      // возможных эксземпляров устройства/модуля

    my_cdev = cdev_alloc();  // выделение памяти под структуру, содержащую все
                             // атрибуты устройства

    cdev_init(my_cdev, &mycdev_fops);  // file_operations
    cdev_add(my_cdev, first, count);  // добавим устройство в дерево устройств

    return 0;
}

static void __exit cleanup_chrdev(void) {
    printk(KERN_INFO "Leaving");

    if (my_cdev) cdev_del(my_cdev);
    unregister_chrdev_region(first, count);
}

module_init(init_chrdev);
module_exit(cleanup_chrdev);

MODULE_LICENSE("GPL");
