#include <stdio.h>
#include "esp_dmx.h"
#include "esp_log.h"
#include "esp_system.h"

#define TX_PIN 17 // the pin we are using to TX with
#define RX_PIN 16 // the pin we are using to RX with
#define EN_PIN 21 // the pin we are using to enable TX on the DMX transceiver

static const char *TAG = "main";

static uint8_t data[DMX_MAX_PACKET_SIZE] = {};
void app_main(void)
{
    const dmx_port_t dmx_num = DMX_NUM_2;

    const dmx_config_t dmx_config = DMX_DEFAULT_CONFIG;
    ESP_ERROR_CHECK(dmx_param_config(dmx_num, &dmx_config));

    ESP_ERROR_CHECK(dmx_set_pin(dmx_num, TX_PIN, RX_PIN, EN_PIN));
    dmx_set_mode(dmx_num, DMX_MODE_READ);

    QueueHandle_t queue;
    // intr_alloc_flags is basically the priority
    ESP_ERROR_CHECK(dmx_driver_install(dmx_num, DMX_MAX_PACKET_SIZE, 1, &queue,
                                       1));
    bool timeout = true;
    uint32_t timer = 0;

    while (1)
    {
        dmx_event_t packet;
        // wait until a packet is received or times out
        if (xQueueReceive(queue, &packet, DMX_PACKET_TIMEOUT_TICK))
        {
            if (packet.status == DMX_OK)
            {
                // print a message upon initial DMX connection
                if (timeout)
                {
                    ESP_LOGI(TAG, "dmx connected");
                    timeout = false; // establish connection!
                }

                // read the packet into the data buffer
                dmx_read_packet(dmx_num, data, packet.size);

                // increment the amount of time that has passed since the last packet
                timer += packet.duration;

                // print a log message every 1 second (1000000 us)
                if (timer >= 10000)
                {
                    ESP_LOG_BUFFER_HEX(TAG, data, 16);
                    timer -= 10000;
                }
            }
            else if (packet.status != DMX_OK)
            {
                // something went wrong receiving data
                ESP_LOGE(TAG, "dmx error");
                continue;
            }
        }
        else if (timeout == false)
        {
            // lost connection
            ESP_LOGW(TAG, "lost dmx signal");
            continue;
        }
    }

    // uninstall the DMX driver
    ESP_LOGI(TAG, "uninstalling DMX driver");
    dmx_driver_delete(dmx_num);

    ESP_LOGI(TAG, "terminating program");
}
