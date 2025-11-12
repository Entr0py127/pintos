#include "threads/fixed-point.h"

/* 정수 -> fixed-point */
fixed_t INT_TO_FP(int n) {
    return (fixed_t)n << FP_SHIFT;
}

/* fixed-point -> 정수 (0으로 내림) */
int FP_TO_INT_ZERO(fixed_t x) {
    return x >> FP_SHIFT;
}

/* fixed-point -> 정수 (반올림) */
int FP_TO_INT_FLOOR(fixed_t x) {
    return (x >= 0) ? (x + (1 << (FP_SHIFT-1))) >> FP_SHIFT
                     : (x - (1 << (FP_SHIFT-1))) >> FP_SHIFT;
}

/* fixed-point + fixed-point */
fixed_t FP_ADD(fixed_t x, fixed_t y) {
    return x + y;
}

/* fixed-point - fixed-point */
fixed_t FP_SUB(fixed_t x, fixed_t y) {
    return x - y;
}

/* fixed-point * fixed-point */
fixed_t FP_MUL(fixed_t x, fixed_t y) {
    return ((int64_t)x * y) >> FP_SHIFT;
}

/* fixed-point / fixed-point */
fixed_t FP_DIV(fixed_t x, fixed_t y) {
    return ((int64_t)x << FP_SHIFT) / y;
}

/* fixed-point + int */
fixed_t FP_ADD_INT(fixed_t x, int n) {
    return x + ((fixed_t)n << FP_SHIFT);
}

/* fixed-point - int */
fixed_t FP_SUB_INT(fixed_t x, int n) {
    return x - ((fixed_t)n << FP_SHIFT);
}

/* fixed-point * int */
fixed_t FP_MUL_INT(fixed_t x, int n) {
    return x * n;
}

/* fixed-point / int */
fixed_t FP_DIV_INT(fixed_t x, int n) {
    return x / n;
}
