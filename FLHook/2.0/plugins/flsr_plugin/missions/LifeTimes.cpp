#include <FLHook.h>
#include "../Plugin.h"
#include "LifeTimes.h"

namespace Missions
{
	struct LifeTime
	{
		float lifeTime;
		float currentLifeTime;
	};
	std::unordered_map<unsigned int, LifeTime> lifeTimesBySolarId;

	float GetSolarLifeTime(const unsigned int objId)
	{
		const auto& entry = lifeTimesBySolarId.find(objId);
		if (entry != lifeTimesBySolarId.end())
		{
			return entry->second.lifeTime;
		}
		return -1.0f;
	}

	void SetSolarLifeTime(const unsigned int objId, const float lifeTime)
	{
		if (lifeTime < 0.0f)
			lifeTimesBySolarId.erase(objId);
		else
		{
			lifeTimesBySolarId[objId].lifeTime = lifeTime;
			lifeTimesBySolarId[objId].currentLifeTime = lifeTime;
		}
	}

	namespace Hooks
	{
		namespace LifeTimes
		{
			float elapsedTimeInSec = 0.0f;
			void __stdcall Elapse_Time_AFTER(float seconds)
			{
				elapsedTimeInSec += seconds;
				if (elapsedTimeInSec < 1.0f)
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}

				const std::unordered_map<uint, LifeTime> currentLifeTimesBySolarId(lifeTimesBySolarId);
				for (auto& solarLifeTimeEntry : currentLifeTimesBySolarId)
				{
					uint systemId;
					pub::SpaceObj::GetSystem(solarLifeTimeEntry.first, systemId);
					struct PlayerData* playerData = 0;
					while (playerData = Players.traverse_active(playerData))
					{
						if (playerData->iSystemID == systemId && playerData->iShipID)
						{
							auto& lifeTimeEntry = lifeTimesBySolarId.at(solarLifeTimeEntry.first);
							if (HkDistance3DByShip(solarLifeTimeEntry.first, playerData->iShipID) > 10000.0f)
							{
								lifeTimeEntry.currentLifeTime -= elapsedTimeInSec;
								if (lifeTimeEntry.currentLifeTime <= 0.0f)
								{
									lifeTimesBySolarId.erase(solarLifeTimeEntry.first);
									if (pub::SpaceObj::ExistsAndAlive(solarLifeTimeEntry.first) == 0)
										pub::SpaceObj::Destroy(solarLifeTimeEntry.first, DestroyType::VANISH);
								}
							}
							else
								lifeTimeEntry.currentLifeTime = lifeTimeEntry.lifeTime;
						}
					}
				}

				elapsedTimeInSec = 0.0f;
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}