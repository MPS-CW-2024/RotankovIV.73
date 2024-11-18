#pragma once
#include "zone.h"

void updateZoneLeds(Zone* zones);
void checkLeaks(Zone* zones);
void handleZoneAlarm(uint8_t zoneIdx, uint8_t hasLeak);