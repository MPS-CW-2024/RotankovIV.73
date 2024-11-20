#pragma once
#include "zone.h"

void checkLeaks(Zone* zones);
void handleZoneAlarm(uint8_t zoneIdx, uint8_t hasLeak);