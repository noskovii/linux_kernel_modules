#include <linux/cdev.h>  // Для создания символьных устройств
#include <linux/fs.h>  // Для работы с функциями файловой системы
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>  // Содержит PAGE_SIZE
#include <linux/uaccess.h>  // Для передачи данных из user-space в ядро

static dev_t first;  // Идентификатор первого устройства в цепочке
static unsigned int count = 1;  // счетчик устройств
static int my_major = 240, my_minor = 0;
static struct cdev
    *my_cdev;  // для определения операций над символьным устройством

#define MYDEV_NAME "mychrdev"
#define KBUF_SIZE (size_t)((10) * PAGE_SIZE)

static int mychrdev_open(struct inode *inode, struct file *file) {
    static char counter = 0;  // кол-во открытий
    char *kbuf = kmalloc(
        KBUF_SIZE,
        GFP_KERNEL);  // выделяем для каждого приложения/открытия свой буфер

    sprintf(kbuf, "%d\0", counter);  // записываем номер сессии
    kbuf[100000] = '1';  // флаг, что не читали сессию

    file->private_data = kbuf;  // file передается из приложения

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
        printk(KERN_INFO "kernel_mooc read buffer in if =  %c",
               device_buffer[0]);
        copy_to_user(buff, device_buffer, 1);
        device_buffer[100000] = '2';
        *ppos = 0;
        return 1;
    }

    if (device_buffer[100000] == '2') {  // флаг что уже читали номер сессии
        device_buffer[100000] = '\0';
        return 0;
    }

    int maxbytes; /* maximum bytes that can be read from ppos to BUFFER_SIZE*/
    int bytes_to_read; /* gives the number of bytes to read*/
    int bytes_read;    /* number of bytes actually read*/

    maxbytes = KBUF_SIZE - *ppos;
    if (maxbytes > length)
        bytes_to_read = length;
    else
        bytes_to_read = maxbytes;

    if (*ppos >= strlen(device_buffer)) {
        printk(KERN_INFO
               "kernel_mooc charDev : Reached the end of the device\n");
        *ppos = 0;
        return 0;
    }

    if (bytes_to_read == 0) {
        printk(KERN_INFO
               "kernel_mooc charDev : Reached the end of the device\n");
        *ppos = 0;
        return 0;
    }

    printk(KERN_INFO
           "kernel_mooc "
           "---------------------------read------------------------------");
    printk(KERN_INFO
           "kernel_mooc read sess = %c: buffer = %s, strlen = %d, "
           "bytes_to_read = %d *ppos = %d",
           device_buffer[0], device_buffer, strlen(device_buffer),
           bytes_to_read, *ppos);
    printk(KERN_INFO "kernel_mooc buffer + *ppos = %s", device_buffer + *ppos);
    printk(KERN_INFO "kernel_mooc copy to US: *%s* strlen = %d",
           device_buffer + *ppos, strlen(device_buffer));

    bytes_read = bytes_to_read - copy_to_user(buff, device_buffer + *ppos,
                                              strlen(device_buffer + *ppos));

    if (bytes_read) bytes_read = strlen(device_buffer + *ppos);

    *ppos += bytes_read;

    printk(KERN_INFO "kernel_mooc bytes_read = %d", bytes_read);
    printk(KERN_INFO
           "kernel_mooc "
           "-------------------------------------------------------------");

    return bytes_read;
}

static ssize_t mychrdev_write(struct file *fp, const char *buff, size_t length,
                              loff_t *ppos) {
    char *device_buffer = fp->private_data;

    int maxbytes; /* maximum bytes that can be read from ppos to BUFFER_SIZE*/
    int bytes_to_write; /* gives the number of bytes to write*/
    int bytes_writen;   /* number of bytes actually writen*/

    maxbytes = KBUF_SIZE - *ppos;
    if (maxbytes > length)
        bytes_to_write = length;
    else
        bytes_to_write = maxbytes;

    printk(KERN_INFO
           "kernel_mooc "
           "---------------------------write-----------------------------");
    printk(KERN_INFO
           "kernel_mooc buff = *%s* strlen = %d len = %d *ppos = %d dev_buf = "
           "*%s* bytes_to_write = %d",
           buff, strlen(buff), length, *ppos, device_buffer, bytes_to_write);

    int strlen_buf = strlen(device_buffer);

    bytes_writen = length - copy_from_user(device_buffer + *ppos, buff, length);

    int new_pos = length > (strlen_buf - *ppos) ? *ppos + length : strlen_buf;
    printk(KERN_INFO "kernel_mooc new_pos = %d", new_pos);
    device_buffer[new_pos] = '\0';

    printk(KERN_INFO "kernel_mooc res buf = %s", device_buffer);
    *ppos = 0;
    printk(KERN_INFO
           "kernel_mooc "
           "-------------------------------------------------------------");

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

    printk(KERN_INFO "kernel_mooc offset = %ld strlen = %d testpos = %ld",
           offset, strlen(device_buffer), (long)testpos);

    testpos = testpos < strlen(device_buffer) ? testpos : strlen(device_buffer);
    testpos = testpos >= 0 ? testpos : 0;

    file->f_pos = testpos;
    printk(KERN_INFO "kernel_mooc Seeking to %ld position", (long)testpos);
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

    first = MKDEV(my_major, my_minor);  // создание ноды на файловой системе
    register_chrdev_region(
        first, count,
        MYDEV_NAME);  // регистрируем регион (множество идентификаторов) для
                      // возможных экземпляров устройства/модуля

    my_cdev = cdev_alloc();  // выделение памяти под структуру, содержащую все
                             // атрибуты устройства

    cdev_init(my_cdev, &mycdev_fops);  // file_operations
    cdev_add(my_cdev, first, count);  // добавим устройство в дерево устройств

    return 0;
}

static void __exit cleanup_chrdev(void) {
    printk(KERN_INFO "Leaving");

    if (my_cdev)  // проверка того, что создавали устройство
        cdev_del(my_cdev);
    unregister_chrdev_region(
        first,
        count);  // освобождаем номер, который зарезирвировали для устройства
}

module_init(init_chrdev);
module_exit(cleanup_chrdev);

MODULE_LICENSE("GPL");
