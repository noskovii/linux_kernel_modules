/*
 * Разработать модуль ядра solution, который реализует интерфейс драйвера
 * символьного устройства с обработчиком команд ioctl. В качестве устройства
 * необходимо использовать уже существующий node /dev/solution_node с
 * major = 240. Драйвер работает в среде, где исключена необходимость
 * синхронизации параллельных попыток доступа к устройству.
 *
 * Обработчики команд ioctl:
 *     - команда SUM_LENGTH, аргумент - указатель на массив char (длиной не
 *       более 20 элементов), действие - подсчитать длину строки,
 *       аккумулировать, вернуть текущую сумму.
 *     - команда SUM_CONTENT, аргумент - указатель на массив char (длиной не
 *       более 20 элементов), действие - сконвертировать в int, аккумулировать и
 *       вернуть текущую сумму.
 *
 * Объявление команд:
 * #define IOC_MAGIC 'k'
 * #define SUM_LENGTH _IOWR(IOC_MAGIC, 1, char*)
 * #define SUM_CONTENT _IOWR(IOC_MAGIC, 2, char*)
 *
 * Результаты работы драйвера должны быть доступны при чтении с символьного
 * устройства /dev/solution_node в следующем формате M N\n, где M - сумма длинн
 * строк, присланных через команду 4444; N - сумма чисел, присланных через
 * команду 5555.
 */

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static dev_t dev;
static unsigned int count = 1;
static int my_major = 240, my_minor = 0;
static struct cdev *my_cdev;

#define MYDEV_NAME "mychrdev"
#define KBUF_SIZE (size_t)((10) * PAGE_SIZE)

#define IOC_MAGIC 'k'

#define SUM_LENGTH _IOWR(IOC_MAGIC, 1, char *)
#define SUM_CONTENT _IOWR(IOC_MAGIC, 2, char *)

static int sum_l = 0;
static int sum_cont = 0;

static long unlocked_ioctl(struct file *filp, unsigned int cmd,
                           unsigned long arg) {
    char *ar = (char *)arg;
    long num;
    switch (cmd) {
        case SUM_LENGTH:
            sum_l += strlen((char *)arg);
            return sum_l;
        case SUM_CONTENT:
            kstrtol((char *)arg, 10, &num);
            sum_cont += num;
            return sum_cont;
        default:
            return 0;
    }

    return 0;
}

static int mychrdev_open(struct inode *inode, struct file *file) {
    static char counter = 0;
    char *kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);

    sprintf(kbuf, "%d\0", counter);
    kbuf[100000] = '1';

    file->private_data = kbuf;
    counter++;

    return 0;
}

static int mychrdev_release(struct inode *inode, struct file *file) {
    char *kbuf = file->private_data;

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

static const struct file_operations mycdev_fops = {
    .owner = THIS_MODULE,
    .read = mychrdev_read,
    .write = mychrdev_write,
    .open = mychrdev_open,
    .release = mychrdev_release,
    .llseek = mychrdev_lseek,
    .unlocked_ioctl = unlocked_ioctl};

static int __init init_chrdev(void) {
    dev = MKDEV(my_major, my_minor);
    register_chrdev_region(dev, count, MYDEV_NAME);

    my_cdev = cdev_alloc();

    cdev_init(my_cdev, &mycdev_fops);
    cdev_add(my_cdev, dev, count);

    return 0;
}

static void __exit cleanup_chrdev(void) {
    if (my_cdev) cdev_del(my_cdev);
    unregister_chrdev_region(dev, count);
}

module_init(init_chrdev);
module_exit(cleanup_chrdev);

MODULE_LICENSE("GPL");
