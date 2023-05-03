/*
 * Разработать модуль ядра solution, который реализует файловый интерфейс
 * драйвера символьного устройства. В качестве устройства необходимо
 * использовать уже существующий node /dev/solution_node с major = 240. Драйвер
 * должен выделять отдельный участок памяти для каждого нового сеанса (под
 * сеансом понимается период между открытием и закрытием файла устройства) и
 * поддерживать операции записи, чтения и изменение указателя текущей позиции
 * (llseek). При этом драйвер должен обеспечивать корректную работу (в ходе
 * каждого сеанса пользователь получает/пишет данные в отдельный, выделенный для
 * данного сеанса буфер) при параллельном выполнении операций чтения, записи,
 * открытия и закрытия файла устройства.
 *
 * В ходе операции записи драйвер должен реализовывать запись строк длиной до
 * 255 символов в текущую позицию указателя.
 *
 * При чтении с символьного устройства /dev/solution_node драйвер должен
 * возвращать данные в следующем формате: S, где S - при первом обращении
 * является порядковым номером сеанса (начиная с нуля), а при последующих -
 * строкой, находящейся на данный момент по указателю позиции для данного
 * сеанса.
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

static int mychrdev_open(struct inode *inode, struct file *file) {
    static char counter = 0;
    char *kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);

    sprintf(kbuf, "%d\0", counter);
    kbuf[100000] = '1';

    file->private_data = kbuf;

    printk(KERN_INFO "Opening device %s:\n\n", MYDEV_NAME);
    counter++;

    printk(KERN_INFO, "counter: %d\n", counter);
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

static ssize_t mychrdev_read(struct file *file, char __user *buff,
                             size_t length, loff_t *ppos) {
    char *device_buffer = file->private_data;

    if (device_buffer[100000] == '1') {
        copy_to_user(buff, device_buffer, 1);
        device_buffer[100000] = '2';
        *ppos = 0;
        return 1;
    }

    if (device_buffer[100000] == '2') {
        device_buffer[100000] = '\0';
        return 0;
    }

    int maxbytes;
    int bytes_to_read;
    int bytes_read;

    maxbytes = KBUF_SIZE - *ppos;
    if (maxbytes > length)
        bytes_to_read = length;
    else
        bytes_to_read = maxbytes;

    if (*ppos >= strlen(device_buffer)) {
        *ppos = 0;
        return 0;
    }

    if (bytes_to_read == 0) {
        *ppos = 0;
        return 0;
    }

    bytes_read = bytes_to_read - copy_to_user(buff, device_buffer + *ppos,
                                              strlen(device_buffer + *ppos));

    if (bytes_read) bytes_read = strlen(device_buffer + *ppos);

    *ppos += bytes_read;

    return bytes_read;
}

static ssize_t mychrdev_write(struct file *fp, const char *buff, size_t length,
                              loff_t *ppos) {
    char *device_buffer = fp->private_data;

    int maxbytes;
    int bytes_to_write;
    int bytes_writen;

    maxbytes = KBUF_SIZE - *ppos;
    if (maxbytes > length)
        bytes_to_write = length;
    else
        bytes_to_write = maxbytes;

    int strlen_buf = strlen(device_buffer);

    bytes_writen = length - copy_from_user(device_buffer + *ppos, buff, length);

    int new_pos = length > (strlen_buf - *ppos) ? *ppos + length : strlen_buf;
    device_buffer[new_pos] = '\0';

    *ppos = 0;

    return bytes_writen;
}

static loff_t mychrdev_lseek(struct file *file, loff_t offset, int orig) {
    loff_t testpos;
    char *device_buffer = file->private_data;

    switch (orig) {
        case SEEK_SET:
            testpos = offset;
            break;
        case SEEK_CUR:
            testpos = file->f_pos + offset;
            break;
        case SEEK_END:
            testpos = strlen(device_buffer) + offset;
            break;
        default:
            return -EINVAL;
    }

    testpos = testpos < strlen(device_buffer) ? testpos : strlen(device_buffer);
    testpos = testpos >= 0 ? testpos : 0;

    file->f_pos = testpos;
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
