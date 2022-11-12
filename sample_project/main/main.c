#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_dmx.h"
#include "esp_log.h"
#include "esp_system.h"
#include "RdmHandler.h"

#define TX_PIN 17 // the pin we are using to TX with
#define RX_PIN 16 // the pin we are using to RX with
#define EN_PIN 21 // the pin we are using to enable TX on the DMX transceiver

static const char *TAG = "main";
static uint8_t data[DMX_MAX_PACKET_SIZE] = {};

void dmxTask(void *unused)
{
    ESP_ERROR_CHECK(dmx_set_pin(DMX_NUM_2, TX_PIN, RX_PIN, EN_PIN));
    ESP_ERROR_CHECK(dmx_driver_install(DMX_NUM_2, DMX_DEFAULT_INTR_FLAGS));
    initRdmHandler(1);
    int count = 0;

    while (true)
    {
        dmx_event_t event;
        size_t ret = 0;
        ret = dmx_receive(DMX_NUM_2, &event, DMX_TIMEOUT_TICK);
        if (ret)
        {
            // Check that no errors occurred.
            if (event.err == ESP_OK)
            {

                if (event.is_rdm)
                {
                    // ESP_LOGI(TAG, "!GOT RDM. SC: %d, size: %d", event.sc, event.size);
                    dmx_read(DMX_NUM_2, data, event.size);
                    handleRdmEvent(&event, data, event.size);
                }

                count++;

                // if ((count % 10) == 0)
                // {
                //     const double currentTimS = esp_timer_get_time() / 1000000.0;
                //     // ESP_LOGE(TAG, "%f: hz: %f, ret: %d, data: %d", currentTimS, (count / currentTimS), ret, data[1]);
                //     // ESP_LOG_BUFFER_HEX(TAG, data, 16);
                // }
            }
            else
            {
                ESP_LOGE(TAG, "dmx error: %s", esp_err_to_name(event.err));
            }
        }
    }
}

void app_main()
{
    TaskHandle_t dmxTaskHandle = NULL;
    // TODO determine real stack usage and reduce later
    xTaskCreatePinnedToCore(dmxTask, "DMX_TASK", 10240, NULL, 2, &dmxTaskHandle, 1);
    if (!dmxTaskHandle)
    {
        ESP_LOGE(TAG, "Failed to create dmx task");
    }
}