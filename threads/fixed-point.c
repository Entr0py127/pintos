#include "threads/fixed-point.h"
#include <stdint.h>

#define FP_SHIFT 14  // 17.14 fixed-point

/* 정수 -> fixed-point */
int int_to_fp(int n) { return n * F; }

/* fixed-point -> 정수*/
int fp_to_int(int n) { return n / F; }

/* fixed-point -> 정수 (round) */
int fp_to_int_round(int x) { return (x >= 0) ? (x + F/2)/F : (x - F/2)/F; }

/* fixed-point + fixed-point */
int fp_add(int x, int y) { return x + y; }

/* fixed-point + int */
int fp_add_int(int x, int n) { return x + n*F; }

/* fixed-point - fixed-point */
int fp_sub(int x, int y) { return x - y; }

/* fixed-point - int */
int fp_sub_int(int x, int n) { return x - n*F; }

/* fixed-point * fixed-point */
int fp_mul(int x, int y) { return ((int64_t)(x)) * y / F; }

/* fixed-point * int */
int fp_mul_int(int x, int n) { return x * n; }

/* fixed-point / fixed-point */
int fp_div(int x, int y) { return ((int64_t)(x)) * F / y; }

/* fixed-point / int */
int fp_div_int(int x, int n) { return x / n; }
