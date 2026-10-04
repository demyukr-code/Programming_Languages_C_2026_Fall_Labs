/*
 * Lab 3, Task 2
 * Name: Kristina Deminska
 * Student ID: 251ADB152
 */

#include <stdio.h>

void swap(int *x, int *y) {
    int temp = *x;
    *x = *y;
    *y = temp;
}

void modify_value(int *x) {
    *x = *x * 2;
}
