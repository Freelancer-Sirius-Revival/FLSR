#include "CndHasCargo.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndHasCargo*> observedCndHasCargo;
	std::vector<CndHasCargo*> orderedCndHasCargo;

	CndHasCargo::CndHasCargo(const ConditionParent& parent, const uint label, const std::unordered_map<uint, uint>& countPerCargo) :
		Condition(parent),
		label(label),
		countPerCargo(countPerCargo)
	{}

	CndHasCargo::~CndHasCargo()
	{
		Unregister();
	}

	void CndHasCargo::Register()
	{
		struct PlayerData* playerData = 0;
		while (playerData = Players.traverse_active(playerData))
		{
			if (Matches(playerData->iOnlineID))
			{
				ExecuteTrigger();
				return;
			}
		}
		if (observedCndHasCargo.insert(this).second)
			orderedCndHasCargo.push_back(this);
	}

	void CndHasCargo::Unregister()
	{
		observedCndHasCargo.erase(this);
		if (const auto it = std::find(orderedCndHasCargo.begin(), orderedCndHasCargo.end(), this); it != orderedCndHasCargo.end())
			orderedCndHasCargo.erase(it);
	}

	bool CndHasCargo::HasCargo(const uint clientId)
	{
		if (!HkIsValidClientID(clientId))
			return false;

		for (const auto& cargoCount : countPerCargo)
		{
			bool found = false;
			for (const auto& equip : Players[clientId].equipDescList.equip)
			{
				if (!equip.bMounted && equip.iArchID == cargoCount.first && equip.iCount >= cargoCount.second)
				{
					found = true;
					break;
				}
			}
			if (!found)
				return false;
		}
		return true;
	}

	bool CndHasCargo::Matches(const uint clientId)
	{
		const auto& mission = missions.at(parent.missionId);
		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId) && HasCargo(clientId))
			{
				activator = MissionObject(MissionObjectType::Client, clientId);
				return true;
			}
			return false;
		}

		if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (labelObject.type == MissionObjectType::Client && clientId == labelObject.id)
				{
					if (HasCargo(labelObject.id))
					{
						activator = labelObject;
						return true;
					}
					return false;
				}
			}
			return false;
		}
		return false;
	}

	static void LoopThroughConditions(const uint clientId)
	{
		const auto currentConditions(orderedCndHasCargo);
		for (const auto& condition : currentConditions)
		{
			if (observedCndHasCargo.contains(condition) && condition->Matches(clientId))
				condition->ExecuteTrigger();
		}
	}

	namespace Hooks
	{
		namespace CndHasCargo
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId)
			{
				LoopThroughConditions(clientId);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall ReqAddItem_AFTER(unsigned int& goodArchetypeId, char* hardpoint, int& count, float& status, bool& mounted, uint clientId)
			{
				LoopThroughConditions(clientId);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall ReqEquipment_AFTER(const EquipDescList& equipDescriptorList, unsigned int clientId)
			{
				LoopThroughConditions(clientId);
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}