#ifndef MAESTRO_ENCODERS_H
#define MAESTRO_ENCODERS_H

#include <stdint.h>

void EncoderInit();
void ReadEncoders(uint8_t *buff);

#endif