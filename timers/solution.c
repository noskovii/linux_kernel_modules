/*
 * Разработайте модуль ядра solution, реализующий настройку и запуск нескольких
 * таймеров по цепочке. Задержка запуска каждого таймера передается в модуль в
 * виде параметра-массива (unsigned long delays[]) в миллисекундах. Задержка для
 * таймера номер N (начиная с нуля) в цепочке должна соответствовать N-ому
 * значению массива delays. Каждая задержка может составлять от 50 мс до 1 с.
 * При запуске первого таймера, а так же при каждом очередном срабатывании
 * обработчика таймера в целях проверки необходимо вызывать функцию check_timer
 * из модуля checker.
 *
 * Пример передачи массива с задержками в модуль:
 *     sudo insmod solution.ko delays=150,500,850,50,1000,350,200
 *
 * Прототип функции для вызова:
 *     void check_timer(void);
 *
 * Прототип задается в файле checker.h .
 */

#include <checker.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/time64.h>
#include <linux/timer.h>

static int a = 0;
module_param(a, int, 0);

static int b = 0;
module_param(b, int, 0);

static unsigned long delays[100];
static int num = 0;
module_param_array(delays, long, &num, S_IRUGO | S_IWUSR);

static int n = 0;
static struct hrtimer hr_timer;

enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer) {
    if (n > num) return HRTIMER_NORESTART;

    ktime_t ktime;
    ktime = ktime_set(0, delays[n] * NSEC_PER_MSEC);
    n++;
    check_timer();
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

    return HRTIMER_RESTART;
}

static int __init example_init(void) {
    ktime_t ktime;
    unsigned long delay_in_ms = 200L;
    ktime = ktime_set(0, delays[0] * NSEC_PER_MSEC);
    hrtimer_init(&hr_timer, CLOCK_REALTIME, HRTIMER_MODE_REL);
    hr_timer.function = &my_hrtimer_callback;
    check_timer();
    n++;
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

    return 0;
}

static void __exit example_exit(void) {
    int ret;
    ret = hrtimer_cancel(&hr_timer);
    if (ret) printk("The timer was still in use...\n");
    printk("HR Timer module uninstalling\n");

    return;
}

module_init(example_init);
module_exit(example_exit);

MODULE_LICENSE("GPL");
