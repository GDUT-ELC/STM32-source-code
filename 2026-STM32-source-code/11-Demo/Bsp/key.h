#ifndef KEY_H
#define KEY_H
#include "main.h"
/* PD0/PD1/PD3/PD4, active low; call regularly, returns one event per press. */
uint8_t key_getnum(void);
#endif
