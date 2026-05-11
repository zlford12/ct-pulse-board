#ifndef MAESTRO_SCAN_H
#define MAESTRO_SCAN_H
#include <stdint.h>

void ScanInit();
void RunScan();
void SetPulseFrequency(float new_freq);
void SetPulseCount(uint16_t new_count);
static void PulseTimer(void *arg);
static void ScanTimeout(void *arg);

#endif //MAESTRO_SCAN_H