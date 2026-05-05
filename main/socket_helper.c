#include "socket_helper.h"
#include "esp_log.h"
#include "lwip/sockets.h"

static constexpr char TAG[] = "socket";
int listen_sock;
int client_sock = -1;

void SocketInit()
{
    bool success = false;

    while (!success)
    {
        ESP_LOGE(TAG, "Creating Socket...");
        listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            return;
        }

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(6969);

        if (bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            closesocket(listen_sock);
            return;
        }

        if (listen(listen_sock, 1) != 0) {
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
            closesocket(listen_sock);
            return;
        }
        ESP_LOGI(TAG, "Socket listening on port 6969");
        success = true;
    }
}

void SocketListen(char *message, int message_size)
{
    if (client_sock >= 0)
    {
        ESP_LOGI(TAG, "Closing existing client socket...");
        SocketClose();
    }
    ESP_LOGI(TAG, "Waiting for a new client connection...");

    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addrlen);
    ESP_LOGI(TAG, "Accepted connection from IP %s", inet_ntoa(client_addr.sin_addr));
    if (client_sock < 0) {
        ESP_LOGI(TAG, "Error occurred during accept: errno %d", errno);
    }
    int bytes_read = read(client_sock, message, message_size - 1);
    if (bytes_read > 0 && message[bytes_read - 1] == '\n')
    {
        message[bytes_read - 1] = '\0';
        bytes_read--;
    }
    ESP_LOGI(TAG, "Received %d bytes from client: %s", bytes_read, message);
}

void SendFrames(const uint32_t *buff, uint32_t size)
{
    //write(client_sock, buff, size);

    uint32_t count = size / sizeof(uint32_t);
    char text_buf[32]; // Buffer for one number string

    for (uint32_t i = 0; i < count; i++) {
        int len = snprintf(text_buf, sizeof(text_buf), "%lu\n", buff[i]);
        write(client_sock, text_buf, len);
    }
}

void SendResponse(const char *response)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s\n", response);
    write(client_sock, buffer, strlen(buffer));
}

void SocketClose()
{
    if (client_sock >= 0)
    {
        shutdown(client_sock, SHUT_RDWR);
        closesocket(client_sock);
        client_sock = -1;
    }
}
