#ifndef CPU_FREQ_H
#define CPU_FREQ_H

#include <Arduino.h>

enum cpu_freq {
	CPU_HIGH = 240,
	CPU_MEDIUM = 160,
	CPU_LOW = 80,
};

extern uint32_t __lastCPUSet;

void setCPU(cpu_freq freq);

#endif