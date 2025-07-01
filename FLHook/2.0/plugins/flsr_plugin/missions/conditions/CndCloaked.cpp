#include "CndCloaked.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndCloaked*> cloakedConditions;

	CndCloaked::CndCloaked(const ConditionParent& parent, const uint objNameOrLabel, const bool cloaked) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		cloaked(cloaked)
	{}

	CndCloaked::~CndCloaked()
	{
		Unregister();
	}

	void CndCloaked::Register()
	{
		cloakedConditions.insert(this);
	}

	void CndCloaked::Unregister()
	{
		cloakedConditions.erase(this);
	}

	bool IsCloaked(uint objId)
	{
		IObjRW* inspect;
		StarSystem* starSystem;
		if (!GetShipInspect(objId, inspect, starSystem) || !(inspect->cobj->objectClass & CObject::CEQOBJ_MASK))
			return false;
		return reinterpret_cast<CEqObj*>(inspect->cobj)->get_cloak_percentage() > 0.9f;
	}

	bool CndCloaked::Matches(const MissionObject object)
	{
		const auto& mission = missions.at(parent.missionId);
		if (object.type == MissionObjectType::Client && objNameOrLabel == Stranger && !mission.clientIds.contains(object.id))
		{
			uint shipId;
			pub::Player::GetShip(object.id, shipId);
			if (shipId && IsCloaked(shipId) == cloaked)
			{
				activator = object;
				return true;
			}
			return false;
		}

		if ((object.type == MissionObjectType::Client && !mission.clientIds.contains(object.id)) ||
			(object.type == MissionObjectType::Object && !mission.objectIds.contains(object.id)))
		{
			return false;
		}

		if (object.type == MissionObjectType::Object)
		{
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				if (objectByName->second == object.id && IsCloaked(object.id) == cloaked)
				{
					activator = object;
					return true;
				}
				return false;
			}
		}
		if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (object == labelObject)
				{
					if (object.type == MissionObjectType::Client)
					{
						uint shipId;
						pub::Player::GetShip(object.id, shipId);
						if (shipId && IsCloaked(shipId) == cloaked)
						{
							activator = object;
							return true;
						}
						return false;
					}
					else
					{
						if (IsCloaked(object.id) == cloaked)
						{
							activator = object;
							return true;
						}
						return false;
					}
				}
			}
		}
		return false;
	}
}