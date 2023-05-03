/*
 * Разработать модуль ядра solution, который реализует файловый интерфейс
 * драйвера символьного устройства. В качестве устройства необходимо
 * использовать уже существующий node /dev/solution_node с major = 240. Драйвер
 * должен вести подсчет общего количества открытий символьного устройства и
 * общего объема записанных в него данных. Драйвер работает в среде, где
 * исключена необходимость синхронизации параллельных попыток доступа к
 * устройству.
 *
 * Результаты работы драйвера должны быть доступны при чтении с символьного
 * устройства /dev/solution_node в следующем формате M N\n, где M - общее
 * количество открытий устройства на чтение или запись, N - общее количество
 * данных, записанных в устройство.
 */

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static dev_t dev;
static unsigned int count = 1;
static int my_major = 240, my_minor = 0;
static struct cdev *my_cdev;

#define MYDEV_NAME "mychrdev"
#define KBUF_SIZE (size_t)((10) * PAGE_SIZE)

static int sum = 0, sum_old = 0, data_sum, data_sum_old = 0;

static int mychrdev_open(struct inode *inode, struct file *file) {
    sum++;

    char *kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);
    file->private_data = kbuf;

    printk(KERN_INFO "Opening device %s:\n\n", MYDEV_NAME);
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

static ssize_t mychrdev_read(struct file *file, char __user *buf, size_t lbuf,
                             loff_t *ppos) {
    if (sum_old == sum && data_sum_old == data_sum) return 0;

    char *tmp = kmalloc(KBUF_SIZE, GFP_KERNEL);
    int num = sprintf(tmp, "%d %d\n", sum, data_sum);
    int nbytes = lbuf - copy_to_user(buf, tmp, num);

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
    printk(KERN_INFO "Loading");

    dev = MKDEV(my_major, my_minor);
    register_chrdev_region(dev, count, MYDEV_NAME);

    my_cdev = cdev_alloc();

    cdev_init(my_cdev, &mycdev_fops);
    cdev_add(my_cdev, dev, count);

    return 0;
}

static void __exit cleanup_chrdev(void) {
    printk(KERN_INFO "Leaving");

    if (my_cdev) cdev_del(my_cdev);
    unregister_chrdev_region(dev, count);
}

module_init(init_chrdev);
module_exit(cleanup_chrdev);

MODULE_LICENSE("GPL");
