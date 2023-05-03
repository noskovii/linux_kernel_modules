/*
 * Разработать модуль ядра solution, который объявит несколько указателей, при
 * загрузке модуля динамически выделит под них необходимое количество памяти и
 * освободит ее при выгрузке.
 *
 * Прототипы всех следующих функций определены в файле checker.h.
 * 1. Указатель произвольного типа
 *     Функция ssize_t get_void_size(void) вернет сгенерированный размер в
 *     байтах, которое необходимо выделить. После выделения памяти необходимо
 *     передать указатель в функцию void submit_void_ptr(void *p) для проверки.
 * 2. Указатель на целочисленный массив
 *     Функция ssize_t get_int_array_size(void) вернет количество элементов,
 *     для которых требуется выделить память. После выделения памяти необходимо
 *     передать указатель в функцию void submit_int_array_ptr(int *p) для
 *     проверки.
 * 3. Указатель на структуру
 *     Требуется выделить память под структуру struct device, после чего
 *     передать указатель в функцию void submit_struct_ptr(struct device *p);
 *     для проверки.
 *
 * Для освобождения выделенной для указателя памяти, при выгрузке модуля
 * необходимо вызвать функцию void checker_kfree(void *p) вместо стандартной
 * void kfree(const void *).
 */

#include <checker.h>
#include <linux/module.h>
#include <linux/slab.h>

void *mem_1;
int *mem_2;
struct device *mem_3;

int init_module(void) {
    int size_1 = get_void_size();
    mem_1 = kmalloc(sizeof(void) * size_1, GFP_KERNEL);
    submit_void_ptr(mem_1);

    int size_2 = get_int_array_size();
    mem_2 = kmalloc(sizeof(int) * size_2, GFP_KERNEL);
    submit_int_array_ptr(mem_2);

    mem_3 = kmalloc(sizeof(struct device), GFP_KERNEL);
    submit_struct_ptr(mem_3);
}

void exit_module(void) {
    checker_kfree(mem_1);
    checker_kfree(mem_2);
    checker_kfree(mem_3);
}

module_exit(exit_module);

MODULE_LICENSE("GPL");
