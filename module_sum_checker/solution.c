/*
 * Разработать модуль ядра solution, который проверит правильность работы
 * функции из модуля ядра checker и выведет в системный журнал результаты
 * проверки с соответствующим уровнем.
 *
 * Функция, работу которой необходимо проверить, определена прототипом в файле
 * checker.h:
 *     int array_sum(short *arr, size_t n);
 * arr - массив чисел, n - его размер, возвращаемое значение - сумма элементов
 * данного массива.
 *
 * Необходимо из разработанного модуля вызвать произвольное число раз (но не
 * менее 10) функцию array_sum() с произвольными массивами и проверить
 * правильность возвращаемого ей значения. Если сумма элементов массива была
 * посчитана верно - вывести в системный журнал сообщение в формате
 *     <result_from_array_sum> <arr[0]> <arr[1]> ... <arr[n-1]>
 * с уровнем info, а если не верно - в таком же формате, но с уровнем err.
 *
 * В файле checker.h также определен макрос CHECKER_MACRO, который необходимо
 * вызвать при загрузке и выгрузке модуля (функции загрузки и выгрузки модуля
 * должны содержать init и exit в названии соответственно).
 *
 * Для форматирования результатов можно воспользоваться вспомогательной
 * функцией, определенной в файле checker.h, которая сохраняет данные в
 * требуемом формате в строку buf и возвращает ее длину: ssize_t
 * generate_output(int sum, short *arr, size_t size, char *buf);
 */

#include <checker.h>
#include <linux/module.h>

int init_module(void) {
    CHECKER_MACRO;

    int i;
    for (i = 0; i < 11; i++) {
        short mas[i];
        int j;
        int sum = 0;

        for (j = 0; j < i; j++) {
            mas[j] = j;
            sum += j;
        }

        int res = array_sum(mas, i);
        printk("kernel_mooc res = %d", res);

        char str[512];
        int size = generate_output(res, mas, i, str);
        str[size] = '\0';

        if (res == sum) {
            printk(KERN_INFO "%s", str);
            printk("kernel_mooc INFO = *%d* %s", sum, str);
        } else {
            printk(KERN_ERR "%s", str);
            printk("kernel_mooc ERR = *%d* %s", sum, str);
        }
    }
}

void exit_module(void) { CHECKER_MACRO; }

module_exit(exit_module);

MODULE_LICENSE("GPL");
