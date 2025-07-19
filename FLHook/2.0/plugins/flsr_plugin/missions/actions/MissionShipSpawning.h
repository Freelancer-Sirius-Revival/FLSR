#pragma once
#include "../Mission.h"

namespace Missions
{
	uint SpawnShip(const uint msnNpcId, Mission& mission, const Vector* positionOverride, const Matrix* orientationOverride);
}