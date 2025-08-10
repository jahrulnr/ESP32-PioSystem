#include "cpu_freq.h"

uint32_t __lastCPUSet = (uint32_t)CPU_LOW;

void setCPU(cpu_freq freq) {
	if (__lastCPUSet == (uint32_t) freq) return;

	__lastCPUSet = (uint32_t)freq;
	setCpuFrequencyMhz(__lastCPUSet);
}