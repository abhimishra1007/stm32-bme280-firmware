#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Starts TIM2 with an update interrupt at the given rate. */
void tim2_init_periodic(uint32_t rate_hz);
/* Returns 1 (and clears the flag) if a period elapsed since the last call. */
int tim2_consume_tick(void);

#endif
