#include "jpegdec_performance.h"

// Static variable definitions
uint32_t JPEGPerformanceMonitor::decodeStartTime = 0;
uint32_t JPEGPerformanceMonitor::totalDecodeTime = 0;
uint32_t JPEGPerformanceMonitor::decodeCount = 0;
uint32_t JPEGPerformanceMonitor::maxDecodeTime = 0;
uint32_t JPEGPerformanceMonitor::minDecodeTime = 0;
size_t JPEGPerformanceMonitor::maxMemoryUsed = 0;
