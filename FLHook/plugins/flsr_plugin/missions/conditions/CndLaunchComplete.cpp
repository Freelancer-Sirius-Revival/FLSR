#include "CndLaunchComplete.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndLaunchComplete*> observedCndLaunchComplete;
	std::vector<CndLaunchComplete*> orderedCndLaunchComplete;

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
		if (observedCndLaunchComplete.insert(this).second)
			orderedCndLaunchComplete.push_back(this);
	}

	void CndLaunchComplete::Unregister()
	{
		observedCndLaunchComplete.erase(this);
		if (const auto it = std::find(orderedCndLaunchComplete.begin(), orderedCndLaunchComplete.end(), this); it != orderedCndLaunchComplete.end())
			orderedCndLaunchComplete.erase(it);
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
				const uint clientId = HkGetClientIDByShip(shipId);
				if (!clientId)
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}

				const auto currentConditions(orderedCndLaunchComplete);
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
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}