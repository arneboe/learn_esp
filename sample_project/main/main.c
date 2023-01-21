#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_dmx.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_rdm_client.h"


#define TX_PIN 17 // the pin we are using to TX with
#define RX_PIN 16 // the pin we are using to RX with
#define EN_PIN 21 // the pin we are using to enable TX on the DMX transceiver


static const char *TAG = "main";
static uint8_t data[DMX_MAX_PACKET_SIZE] = {};
static bool identifyOn = false;

void handleRdmEvent(const dmx_packet_t *dmxPacket, const void *data, const uint16_t size);

void startAddressChangedCb(uint16_t newAddr)
{
    ESP_LOGI(TAG, "Start addr callback: %d", newAddr);
}

void personalityCb(uint8_t person)
{
    ESP_LOGI(TAG, "personalityCb callback: %d", person);
}

void dmxTask(void *unused)
{
    ESP_ERROR_CHECK(dmx_set_pin(DMX_NUM_2, TX_PIN, RX_PIN, EN_PIN));
    ESP_ERROR_CHECK(dmx_driver_install(DMX_NUM_2, DMX_DEFAULT_INTR_FLAGS));

    // TODO use SP_ERROR_CHECK
    rdm_client_init(DMX_NUM_2, 42, 13, "LICHTAUSGANG Blinder", "default");
    rdm_client_add_personality(DMX_NUM_2, 42, "other footprint");
    rdm_client_set_start_address_changed_cb(DMX_NUM_2, startAddressChangedCb);
    rdm_client_set_personality_changed_cb(DMX_NUM_2, personalityCb);
    while (true)
    {
        dmx_packet_t dmxPacket;
        size_t ret = 0;
        ret = dmx_receive(DMX_NUM_2, &dmxPacket, DMX_TIMEOUT_TICK);
        if (ret)
        {
            if (dmxPacket.err == ESP_OK)
            {
                if (dmxPacket.is_rdm)
                {
                    dmx_read(DMX_NUM_2, data, dmxPacket.size);
                    rdm_client_handle_rdm_message(DMX_NUM_2, &dmxPacket, data, dmxPacket.size);
                }
            }
            else
            {
                ESP_LOGE(TAG, "dmx error: %s", esp_err_to_name(dmxPacket.err));
            }
        }
    }
}

void app_main()
{
    TaskHandle_t dmxTaskHandle = NULL;
    xTaskCreatePinnedToCore(dmxTask, "DMX_TASK", 10240, NULL, 2, &dmxTaskHandle, 1);
    if (!dmxTaskHandle)
    {
        ESP_LOGE(TAG, "Failed to create dmx task");
    }
}