#ifndef MAESTRO_SCAN_H
#define MAESTRO_SCAN_H
#include <stdint.h>

void ScanInit();
void RunScan();
void SetPulseFrequency(uint16_t new_freq);
void SetRoi(uint32_t min, uint32_t max);
static void PulseTimer(void *arg);
static void RoiTimeout(void *arg);

#endif //MAESTRO_SCAN_H