#pragma once
#include <esp_rdm.h>

void initRdmHandler(uint16_t dmxAddr);

void handleRdmEvent(const dmx_event_t *event, const void *data, const uint16_t size);