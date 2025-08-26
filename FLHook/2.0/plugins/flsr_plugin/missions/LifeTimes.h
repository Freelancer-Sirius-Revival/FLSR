#pragma once
#include <unordered_map>

namespace Missions
{
	float GetSolarLifeTime(const unsigned int objId);
	void SetSolarLifeTime(const unsigned int objId, const float lifeTime);

	namespace Hooks
	{
		namespace LifeTimes
		{
			void __stdcall Elapse_Time_AFTER(float seconds);
		}
	}
}