#ifndef MAESTRO_SOCKET_HELPER_H
#define MAESTRO_SOCKET_HELPER_H
#include <stdint.h>

void SocketInit();
void SocketListen(char *message, int message_size);
void SendFrames(const uint32_t *buff, uint32_t size);
void SendResponse(const char *response);
void SocketClose();

#endif