#include "CndHealthDec.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndHealthDec*> observedCndHealthDec;
	std::vector<CndHealthDec*> orderedCndHealthDec;

	CndHealthDec::CndHealthDec(const ConditionParent& parent,
								const uint objNameOrLabel,
								const float remainingHitpoints,
								const std::unordered_set<uint> colGrpIds,
								const bool damagedIsActivator) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		remainingHitpoints(remainingHitpoints),
		colGrpIds(colGrpIds),
		damagedIsActivator(damagedIsActivator)
	{}

	CndHealthDec::~CndHealthDec()
	{
		Unregister();
	}

	void CndHealthDec::Register()
	{
		if (observedCndHealthDec.insert(this).second)
			orderedCndHealthDec.push_back(this);
	}

	void CndHealthDec::Unregister()
	{
		observedCndHealthDec.erase(this);
		if (const auto it = std::find(orderedCndHealthDec.begin(), orderedCndHealthDec.end(), this); it != orderedCndHealthDec.end())
			orderedCndHealthDec.erase(it);
	}

	bool CndHealthDec::Matches(const IObjRW* damagedObject, const float incomingDamage, const DamageList* damageList, const CArchGroup* hitColGrp)
	{
		if (hitColGrp == nullptr)
		{
			if (!colGrpIds.contains(RootGroup) || (damagedObject->cobj->get_max_hit_pts() * remainingHitpoints < damagedObject->cobj->hitPoints - incomingDamage))
				return false;
		}
		else
		{
			if (!colGrpIds.contains(CreateID(hitColGrp->colGrp->name.value)) || hitColGrp->GetMaxHitPoints() * remainingHitpoints < hitColGrp->hitPts - incomingDamage)
				return false;
		}

		const MissionObject inflictorObj = damageList->is_inflictor_a_player()
											? MissionObject(MissionObjectType::Client, HkGetClientIDByShip(damageList->get_inflictor_id()))
											: MissionObject(MissionObjectType::Object, damageList->get_inflictor_id());

		MissionObject cndActivator = inflictorObj;
		if (damagedIsActivator)
		{
			activator = damagedObject->is_player()
				? MissionObject(MissionObjectType::Client, damagedObject->cobj->ownerPlayer)
				: MissionObject(MissionObjectType::Object, damagedObject->cobj->id);
		}

		const auto& mission = missions.at(parent.missionId);
		if (!damagedObject->is_player())
		{
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				if (objectByName->second == damagedObject->cobj->id)
				{
					activator = cndActivator;
					return true;
				}
				return false;
			}
		}
		if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			const MissionObject object = damagedObject->is_player()
											? MissionObject(MissionObjectType::Client, damagedObject->cobj->ownerPlayer)
											: MissionObject(MissionObjectType::Object, damagedObject->get_id());
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (object == labelObject)
				{
					activator = cndActivator;
					return true;
				}
			}
		}
		return false;
	}

	static void LoopThroughConditions(const IObjRW* damagedObject, const float incomingDamage, const DamageList* damageList, const CArchGroup* hitColGrp)
	{
		// Ignore any damage caused by repair guns.
		if (incomingDamage <= 0.0f)
			return;

		const auto currentConditions(orderedCndHealthDec);
		for (const auto& condition : currentConditions)
		{
			if (observedCndHealthDec.contains(condition) && condition->Matches(damagedObject, incomingDamage, damageList, hitColGrp))
				condition->ExecuteTrigger();
		}
	}

	namespace Hooks
	{
		namespace CndHealthDec
		{
			void __stdcall ShipAndSolarColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, hitColGrp);
			}

			void __stdcall ShipAndSolarHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, nullptr);
			}
		}
	}
}