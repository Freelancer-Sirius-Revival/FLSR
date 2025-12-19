#include "CndHealthInc.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	const uint RootGroup = CreateID("root");

	std::unordered_set<CndHealthInc*> observedCndHealthInc;
	std::vector<CndHealthInc*> orderedCndHealthInc;

	CndHealthInc::CndHealthInc(const ConditionParent& parent,
		const uint objNameOrLabel,
		const float relativeHitpointsThreshold,
		const std::unordered_set<uint> colGrpIds,
		const bool repairedIsActivator) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		relativeHitpointsThreshold(relativeHitpointsThreshold),
		colGrpIds(colGrpIds),
		repairedIsActivator(repairedIsActivator)
	{}

	CndHealthInc::~CndHealthInc()
	{
		Unregister();
	}

	void CndHealthInc::Register()
	{
		if (observedCndHealthInc.insert(this).second)
			orderedCndHealthInc.push_back(this);
	}

	void CndHealthInc::Unregister()
	{
		observedCndHealthInc.erase(this);
		if (const auto it = std::find(orderedCndHealthInc.begin(), orderedCndHealthInc.end(), this); it != orderedCndHealthInc.end())
			orderedCndHealthInc.erase(it);
	}

	bool CndHealthInc::Matches(const IObjRW* damagedObject, const float incomingDamage, const DamageList* damageList, const CArchGroup* hitColGrp)
	{
		if (hitColGrp == nullptr)
		{
			if (!colGrpIds.contains(RootGroup) || (damagedObject->cobj->get_max_hit_pts() * relativeHitpointsThreshold > damagedObject->cobj->hitPoints - incomingDamage))
				return false;
		}
		else
		{
			if (!colGrpIds.contains(CreateID(hitColGrp->colGrp->name.value)) || hitColGrp->GetMaxHitPoints() * relativeHitpointsThreshold > hitColGrp->hitPts - incomingDamage)
				return false;
		}

		const MissionObject inflictorObj = damageList->is_inflictor_a_player()
			? MissionObject(MissionObjectType::Client, HkGetClientIDByShip(damageList->get_inflictor_id()))
			: MissionObject(MissionObjectType::Object, damageList->get_inflictor_id());

		MissionObject cndActivator = inflictorObj;
		if (repairedIsActivator)
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
		// Only allow damage caused by repair guns.
		if (incomingDamage >= 0.0f)
			return;

		const auto currentConditions(orderedCndHealthInc);
		for (const auto& condition : currentConditions)
		{
			if (observedCndHealthInc.contains(condition) && condition->Matches(damagedObject, incomingDamage, damageList, hitColGrp))
				condition->ExecuteTrigger();
		}
	}

	namespace Hooks
	{
		namespace CndHealthInc
		{
			void __stdcall ShipAndSolarColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList)
			{
				LoopThroughConditions(damagedObject, incomingDamage, damageList, hitColGrp);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall ShipAndSolarHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList)
			{
				LoopThroughConditions(damagedObject, incomingDamage, damageList, nullptr);
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}