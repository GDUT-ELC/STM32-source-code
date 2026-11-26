#ifndef HCSR04_H
#define HCSR04_H
#include "main.h"
#include "tim.h"
typedef struct {
    volatile uint8_t edge_state; /* 0 idle, 1 rising expected, 2 falling expected */
    volatile uint8_t valid;
    volatile uint32_t ring_ccr;
    volatile uint32_t high_time;
    volatile uint32_t samples;
    volatile uint32_t timeouts;
    volatile float distance;
    uint32_t started_ms;
} HCSR04Info;
extern HCSR04Info HCSR04;
void hcsr04_start(void);
void hcsr04_process(void);
void hcsr04_echo_irq(void);
float hcsr04_getdistance(HCSR04Info *sensor);
#endif
