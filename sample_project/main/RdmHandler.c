#include "RdmHandler.h"
#include <string.h> //FIXME needs to be included because rdm_encode needs memcpy?!
#include <esp_rdm.h>
#include "esp_log.h"
uint16_t dmxAddr = 0;

void initRdmHandler(uint16_t addr)
{
    dmxAddr = addr;
}

void handleRdmEvent(const dmx_event_t *event, const void *data, const uint16_t size)
{
    rdm_header_t header;
    if (rdm_get_header(&header, data))
    {
        if (rdm_is_directed_at_us(DMX_NUM_2, &header))
        {
            if (header.cc == RDM_CC_DISC_COMMAND)
            {
                if (header.pid == RDM_PID_DISC_UNIQUE_BRANCH)
                {

                    // parameter data starts at data[24]

                    //  ESP_LOGI("RDM", " Received RDM discovery message");
                    // ESP_LOG_BUFFER_HEX("RDM", data, event->size);

                    const rdm_uid_t lowUid = buf_to_uid(data + 24); // TODO check if buffer is large enough
                    const rdm_uid_t highUid = buf_to_uid(data + 30);

                    const rdm_uid_t ourUid = rdm_get_uid(DMX_NUM_2);

                    if (!rdm_is_muted(DMX_NUM_2) && lowUid <= ourUid && ourUid <= highUid)
                    {
                        const size_t respSize = rdm_send_disc_response(DMX_NUM_2, 7, ourUid); // FIXME DMX NUM
                        ESP_LOGI("RDM", " Sent response. %d bytes", respSize);
                        ESP_LOGI("RDM", " low  uid %lld ", lowUid);
                        ESP_LOGI("RDM", " high uid %lld ", highUid);
                    }
                }
                else if (header.pid == RDM_PID_DISC_UN_MUTE)
                {
                    ESP_LOGI("RDM", " Received UNMUTE");
                    rdm_set_muted(DMX_NUM_2, false);
                }
                else if (header.pid == RDM_PID_DISC_MUTE)
                {
                    ESP_LOGI("RDM", " Received MUTE");
                    rdm_set_muted(DMX_NUM_2, true);
                    const size_t bytesSent = rdm_send_mute_response(DMX_NUM_2, header.source_uid, header.tn);
                    ESP_LOGI("RDM", " Responded to mute request with %d bytes", bytesSent);
                    // TODO need to send response
                }
                else
                {
                    ESP_LOGI("RDM", " Received unknown pid: %d", header.pid);
                }
            }
            else
            {
                ESP_LOGI("RDM", " Received unknown rdm");
                ESP_LOG_BUFFER_HEX("RDM", data, event->size);
            }
        }
        else
        {
            ESP_LOGI("RDM", " Received rdm not directed at us");
            ESP_LOG_BUFFER_HEX("RDM", data, event->size);
        }
    }
}