#include "cpu_freq.h"

cpu_freq __lastCPUSet = CPU_LOW;

void setCPU(cpu_freq freq) {
	if (__lastCPUSet == freq) return;

	__lastCPUSet = freq;
	setCpuFrequencyMhz(__lastCPUSet);
}