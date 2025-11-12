#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>

/* Fixed-point 형식: 17.14 */
typedef int64_t fixed_t;
#define FP_SHIFT 14

/* 정수 <-> fixed-point 변환 */
fixed_t INT_TO_FP(int n);
int FP_TO_INT_ZERO(fixed_t x);
int FP_TO_INT_FLOOR(fixed_t x);

/* fixed-point 간 연산 */
fixed_t FP_ADD(fixed_t x, fixed_t y);
fixed_t FP_SUB(fixed_t x, fixed_t y);
fixed_t FP_MUL(fixed_t x, fixed_t y);
fixed_t FP_DIV(fixed_t x, fixed_t y);

/* fixed-point와 정수 간 연산 */
fixed_t FP_ADD_INT(fixed_t x, int n);
fixed_t FP_SUB_INT(fixed_t x, int n);
fixed_t FP_MUL_INT(fixed_t x, int n);
fixed_t FP_DIV_INT(fixed_t x, int n);

#endif /* THREADS_FIXED_POINT_H */
