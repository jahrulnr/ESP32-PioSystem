#include "tasks.h"
#include "mic_config.h"
#include "micbar.h"

TaskHandle_t microphoneListenerTaskHandle = nullptr;
AnalogMicrophone* analogMicrophone = nullptr;
MicBar* micbar = nullptr;

void microphoneListenerTask(void* param) {
  TickType_t lastWakeTime = xTaskGetTickCount();
	int frequency = 100;
	const TickType_t micListenFrequency = pdMS_TO_TICKS(frequency);

	if (!analogMicrophone){
		analogMicrophone = 
			new AnalogMicrophone(MIC_ANALOG_PIN, MIC_GAIN_PIN, MIC_AR_PIN);
		analogMicrophone->init(MIC_BITS_PER_SAMPLE);
	}

	if (!micbar){
		micbar = new MicBar(displayManager.getTFT());	
	}

	while(true) {
    // Wait for the next cycle
    vTaskDelayUntil(&lastWakeTime, micListenFrequency);

		if (displayMutex == nullptr) {
			delay(5000);
			continue;
		}
	
		// Update the display if we can get the mutex
		if (xSemaphoreTake(displayMutex, micListenFrequency) == pdTRUE) {
			micbar->drawBar(analogMicrophone->readPeakLevel(frequency));
			xSemaphoreGive(displayMutex);
		}
	}
}