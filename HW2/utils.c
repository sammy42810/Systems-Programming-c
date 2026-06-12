/*******************************************************************************
 * Name        : utils.c
 * Author      : Samantha Bryan
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include "utils.h"

int cmpr_float(void *x, void *y)
{
    float *a = (float *)x;
    float *b = (float *)y;

    if (*a > *b)
    {
        return 1;
    }
    else if (*a < *b)
    {
        return -1;
    }
    return 0;
}

int cmpr_int(void *x, void *y)
{
    int *a = (int *)x;
    int *b = (int *)y;

    if (*a > *b)
    {
        return 1;
    }
    else if (*a < *b)
    {
        return -1;
    }
    return 0;
}

void print_int(void *x)
{
    int *a = (int *)x;
    printf("%d ", *a);
}

void print_float(void *x)
{
    float *a = (float *)x;
    printf("%f ", *a);
}