#include "CndLaunchComplete.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndLaunchComplete*> observedCndLaunchComplete;

	CndLaunchComplete::CndLaunchComplete(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& baseIds) :
		Condition(parent),
		label(label),
		baseIds(baseIds)
	{}

	CndLaunchComplete::~CndLaunchComplete()
	{
		Unregister();
	}

	void CndLaunchComplete::Register()
	{
		observedCndLaunchComplete.insert(this);
	}

	void CndLaunchComplete::Unregister()
	{
		observedCndLaunchComplete.erase(this);
	}

	bool CndLaunchComplete::Matches(const uint clientId, const uint currentBaseId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && (baseIds.empty() || baseIds.contains(currentBaseId)))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				return true;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId && (baseIds.empty() || baseIds.contains(currentBaseId)))
				{
					activator = object;
					return true;
				}
			}
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndLaunchComplete
		{
			void __stdcall LaunchComplete_AFTER(unsigned int launchObjId, unsigned int shipId)
			{
				returncode = DEFAULT_RETURNCODE;

				const uint clientId = HkGetClientIDByShip(shipId);
				if (!clientId)
					return;

				const std::unordered_set<Missions::CndLaunchComplete*> currentConditions(observedCndLaunchComplete);
				for (const auto& condition : currentConditions)
				{
					IObjRW* inspect;
					StarSystem* system;
					if (observedCndLaunchComplete.contains(condition) &&
						GetShipInspect(launchObjId, inspect, system) &&
						(inspect->cobj->objectClass & CObject::CEQOBJ_MASK) &&
						condition->Matches(clientId, static_cast<CEqObj*>(inspect->cobj)->dockWithBaseId))
					{
						condition->ExecuteTrigger();
					}
				}
			}
		}
	}
}