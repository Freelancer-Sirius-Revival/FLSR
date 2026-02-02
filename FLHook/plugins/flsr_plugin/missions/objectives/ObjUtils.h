#pragma once
#include <FLHook.h>
#include "../Mission.h"

bool FindObjectByNameOrFirstPlayerByLabel(const Missions::Mission& mission, const uint targetObjNameOrPlayerLabel, uint& foundObjId);