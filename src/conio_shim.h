// conio_shim.h
#ifndef CONIO_SHIM_H
#define CONIO_SHIM_H
#include <stdio.h>


static inline void clrscr(void) { printf("\033[2J\033[H"); }
static inline void gotoxy(int x, int y) { printf("\033[%d;%dH", y, x); }


static inline void textcolor(int color) {
    switch(color) {
        case 11: printf("\033[1;36m"); break; // Light Cyan
        case 15: printf("\033[1;37m"); break; // White
        case 7:  printf("\033[0;37m"); break; // Light Gray / Reset
        default: printf("\033[0m");    break; // Default Reset
    }
}

#endif
