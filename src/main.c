
#include "espressif/esp_common.h"
#include <esp/uart.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/sockets.h"

#include "ws2812_i2s/ws2812_i2s.h"

#include "config.h"

#define LED 16



void blink_pwr(void* pvParameters)
{
    gpio_enable(LED, GPIO_OUTPUT);

    while(1) {
        gpio_write(LED, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_write(LED, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


/*
 * Decode CRAP and fill WS2812 buffer
 */
void crap_update(ws2812_pixel_t* ws2812_buf,
                 const char* crap_buf,
                 size_t crap_len)
{
    size_t len = crap_len / 3;
    if (len > CRAP_MAX_LEDS) {
        len = CRAP_MAX_LEDS;
    }

    // Fill buffer
    for (int i = 0; i < CRAP_MAX_LEDS; i++) {
        ws2812_buf[i].red = crap_buf[i*3];
        ws2812_buf[i].green = crap_buf[i*3+1];
        ws2812_buf[i].blue = crap_buf[i*3+2];
    }

    // Write pixels
    ws2812_i2s_update(ws2812_buf, PIXEL_RGB);
}


/*
 * Listen on UDP socket for crap data
 */
void receive_udp(void* pvParameters)
{
    int socket;
    int ret;
    struct sockaddr_in saddr_in;

    memset(&saddr_in, 0, sizeof(struct sockaddr_in));
    saddr_in.sin_family = AF_INET;
    saddr_in.sin_addr.s_addr = inet_addr("0.0.0.0");
    saddr_in.sin_port = htons(CRAP_PORT);

    // Open socket
    socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket == -1) {
        printf("Could not open socket\n");
        return;
    }

    ret = bind(socket,
               (struct sockaddr*)&saddr_in,
               sizeof(struct sockaddr_in));
    if (ret == -1) {
        printf("Could not bind socket\n");
        return;
    }

    // Allocate buffers
    char buf[CRAP_MAX_LEDS * 3];
    ws2812_pixel_t led_buffer[CRAP_MAX_LEDS];
    memset(led_buffer, 0,
           CRAP_MAX_LEDS * sizeof(ws2812_pixel_t));

    while(42) {
        memset(buf, 0, sizeof(buf));
        ret = recv(socket, buf, CRAP_MAX_LEDS * 3, 0);
        if(ret > 0) {
            crap_update((ws2812_pixel_t*)&led_buffer,
                        (const char*)&buf, ret);
        }
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    // Add wifi status LED.
    sdk_wifi_status_led_install(2,
                                PERIPHS_IO_MUX_GPIO2_U,
                                FUNC_GPIO2);

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };

    // Connect to wifi
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    // Initialize WS2812
    ws2812_i2s_init(CRAP_MAX_LEDS, PIXEL_RGB);

    printf("Starting blink task");
    xTaskCreate(blink_pwr,
                "blink_pwr", 256, NULL, 2, NULL);

    printf("Starting UDP listener");
    xTaskCreate(receive_udp,
                "receive_udp", 4096, NULL, 2, NULL);

}

