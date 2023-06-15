/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ulp_riscv_print.h"
#include "string.h"
typedef struct {
    putc_fn_t putc; // Putc function of the underlying driver, e.g. UART
    void *putc_ctx; // Context passed to the putc function
} ulp_riscv_print_ctx_t;

static ulp_riscv_print_ctx_t s_print_ctx;

void ulp_riscv_print_install(putc_fn_t putc, void * putc_ctx)
{
    s_print_ctx.putc_ctx = putc_ctx;
    s_print_ctx.putc = putc;
}

void ulp_riscv_print_str(const char *str)
{
    if (!s_print_ctx.putc) {
        return;
    }

    for (int i = 0; str[i] != 0; i++) {
        s_print_ctx.putc(s_print_ctx.putc_ctx ,str[i]);
    }
}

char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void ulp_riscv_print_int(int h){
    char buf[32] = {0};
    itoa(h, buf,10);
    for (int i = 0; i < buf[i]!=0; i++)
    {
        s_print_ctx.putc(s_print_ctx.putc_ctx ,buf[i]);
    }
    

}

void ulp_riscv_print_hex(int h)
{
    int x;
    int c;

    if (!s_print_ctx.putc) {
        return;
    }

    // Does not print '0x', only the digits (8 digits to print)
    for (x = 0; x < 8; x++) {
        c = (h >> 28) & 0xf; // extract the leftmost byte
        if (c < 10) {
            s_print_ctx.putc(s_print_ctx.putc_ctx ,'0' + c);
        } else {
            s_print_ctx.putc(s_print_ctx.putc_ctx ,'a' + c - 10);
        }
        h <<= 4; // move the 2nd leftmost byte to the left, to be extracted next
    }
}
