#pragma once
#include <stdint.h>

void timer_init(uint64_t interval);
void timer_next(void);