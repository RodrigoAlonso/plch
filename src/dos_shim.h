#ifndef DOS_SHIM_H
#define DOS_SHIM_H
//#include <unistd.h>
//#include <stdlib.h>
//s#include <time.h>

/*static inline void delay(unsigned int ms) {
    usleep(ms * 1000);
}*/

static inline void sound(unsigned int freq) { (void)freq; }
static inline void nosound(void) {}

#endif
