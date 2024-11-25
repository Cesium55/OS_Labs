#ifndef SUM_LIB_H
#define SUM_LIB_H

#include <stdint.h>

struct SumArgs {
    int *array;
    int begin;
    int end;
};

// Функция для подсчёта суммы
int Sum(const struct SumArgs *args);

#endif // SUM_LIB_H
