#include <linux/cdev.h>  // для создания символьных устройств
#include <linux/device.h>
#include <linux/fs.h>  // для работы с функциями файловой системы
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>  // содержит PAGE_SIZE
#include <linux/uaccess.h>  // для передачи данных из user-space в ядро

static dev_t first;  // идентификатор первого устройства в цепочке
static unsigned int count = 1;  // счетчик устройств
static int my_major = 700, my_minor = 0;
static struct cdev
    *my_cdev;  // для определения операций над символьным устройством

#define MYDEV_NAME "mychrdev"
#define KBUF_SIZE (size_t)((10) * PAGE_SIZE)

static struct class *my_class;

#define FIXLEN 1000
static char module_name[FIXLEN] =
    "";  // имена параметра и переменной различаются
module_param_string(node_name, module_name, sizeof(module_name), 0);

static int mychrdev_open(struct inode *inode, struct file *file) {
    static int counter = 0;  // кол-во открытий
    char *kbuf = kmalloc(
        KBUF_SIZE,
        GFP_KERNEL);  // выделяем для каждого приложения/открытия свой буфер
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

static bool first_req = true;
// __user - параметр, пришедший из user-space
static ssize_t mychrdev_read(struct file *file, char __user *buf, size_t lbuf,
                             loff_t *ppos) {
    /*  char *kbuf = file->private_data;

      int nbytes = lbuf - copy_to_user(buf, kbuf + *ppos, lbuf); // copy_to_user
      вернет 0 если все скопировано, если не 0 - то покажет сколько не
      скопировано *ppos += nbytes;

      printk(KERN_INFO "Read device %s nbytes = %d, ppos = %d:\n\n", MYDEV_NAME,
      nbytes, (int)*ppos);

      return nbytes;*/

    if (!first_req) return 0;

    char *tmp = kmalloc(KBUF_SIZE, GFP_KERNEL);
    int size_of_message = sprintf(tmp, "%d\n", my_major);
    int nbytes =
        lbuf -
        copy_to_user(
            buf, tmp,
            size_of_message);  // copy_to_user вернет 0 если все скопировано,
                               // если не 0 - то покажет сколько не скопировано
    first_req = false;
    // sum_old = sum;
    // data_sum_old = data_sum;

    return nbytes;

    /* int error_count = 0;
     // copy_to_user has the format ( * to, *from, size) and returns 0 on
     success error_count = copy_to_user(buf, tmp, size_of_message);

     if (error_count==0){            // if true then have success
        printk(KERN_INFO "EBBChar: Sent %d characters to the user\n",
     size_of_message); return (size_of_message=0);  // clear the position to the
     start and return 0
     }
     else {
        printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n",
     error_count); return -EFAULT;              // Failed -- return a bad
     address message (i.e. -14)
     }*/
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
    // kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL) ; // выделяем память из
    // стандартного пула ядра

    first = MKDEV(my_major, my_minor);  // создание ноды на файловой системе
    register_chrdev_region(
        first, count,
        MYDEV_NAME);  // регистрируем регион (множество идентификаторов) для
                      // возможных экземпляров устройства/модуля

    my_cdev = cdev_alloc();  // выделение памяти под структуру, содержащую все
                             // атрибуты устройства

    cdev_init(my_cdev, &mycdev_fops);  // file_operations
    cdev_add(my_cdev, first, count);  // добавим устройство в дерево устройств

    my_class = class_create(THIS_MODULE, "my_class");
    device_create(my_class, NULL, first, "%s", module_name);
    printk(KERN_INFO "Create device class %s", MYDEV_NAME);

    return 0;
}

static void __exit cleanup_chrdev(void) {
    printk(KERN_INFO "Leaving");

    device_destroy(my_class, first);
    class_destroy(my_class);

    if (my_cdev)  // проверка того, что создавали устройство
        cdev_del(my_cdev);
    unregister_chrdev_region(
        first,
        count);  // освобождаем номер, который зарезирвировали для устройства
}

module_init(init_chrdev);
module_exit(cleanup_chrdev);

MODULE_LICENSE("GPL");
