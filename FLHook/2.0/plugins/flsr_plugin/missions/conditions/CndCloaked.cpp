#include "CndCloaked.h"
#include "../Mission.h"

namespace Missions
{
	std::unordered_set<CndCloaked*> cloakedConditions;

	CndCloaked::CndCloaked(const ConditionParent& parent, const CndCloakedArchetypePtr conditionArchetype) :
		Condition(parent, ConditionType::Cnd_BaseEnter),
		archetype(conditionArchetype)
	{
	}

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
		auto& mission = missions.at(parent.missionId);
		if (object.type == MissionObjectType::Client && archetype->objNameOrLabel == Stranger && !mission.clientIds.contains(object.id))
		{
			uint shipId;
			pub::Player::GetShip(object.id, shipId);
			if (shipId && IsCloaked(shipId) == archetype->cloaked)
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
			if (const auto& objectByName = mission.objectIdsByName.find(archetype->objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				if (objectByName->second == object.id && IsCloaked(object.id) == archetype->cloaked)
				{
					activator = object;
					return true;
				}
				return false;
			}
		}
		if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (object == labelObject)
				{
					if (object.type == MissionObjectType::Client)
					{
						uint shipId;
						pub::Player::GetShip(object.id, shipId);
						if (shipId && IsCloaked(shipId) == archetype->cloaked)
						{
							activator = object;
							return true;
						}
						return false;
					}
					else
					{
						if (IsCloaked(object.id) == archetype->cloaked)
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