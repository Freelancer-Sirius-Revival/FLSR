#include "CndHealthDec.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndHealthDec*> registeredHullConditions;
	std::unordered_set<CndHealthDec*> registeredColGrpConditions;

	CndHealthDec::CndHealthDec(const ConditionParent& parent, const uint objNameOrLabel, const float remainingHitpoints, const std::unordered_set<uint> colGrpIds) :
		Condition(parent),
		objNameOrLabel(objNameOrLabel),
		remainingHitpoints(remainingHitpoints),
		colGrpIds(colGrpIds)
	{}

	CndHealthDec::~CndHealthDec()
	{
		Unregister();
	}

	void CndHealthDec::Register()
	{
		if (colGrpIds.contains(RootGroup))
			registeredHullConditions.insert(this);
		if (colGrpIds.size() > 1 || !colGrpIds.contains(RootGroup))
			registeredColGrpConditions.insert(this);
	}

	void CndHealthDec::Unregister()
	{
		registeredHullConditions.erase(this);
		registeredColGrpConditions.erase(this);
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

		const MissionObject activatorObj = damageList->is_inflictor_a_player()
											? MissionObject(MissionObjectType::Client, damageList->get_inflictor_owner_player())
											: MissionObject(MissionObjectType::Object, damageList->get_inflictor_id());

		const auto& mission = missions.at(parent.missionId);
		if (!damagedObject->is_player())
		{
			if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				if (objectByName->second == damagedObject->cobj->id)
				{
					activator = activatorObj;
					return true;
				}
				return false;
			}
		}
		if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
		{
			const MissionObject object = damagedObject->is_player()
											? MissionObject(MissionObjectType::Client, damagedObject->cobj->ownerPlayer)
											: MissionObject(MissionObjectType::Object, damagedObject->cobj->id);
			for (const auto& labelObject : objectsByLabel->second)
			{
				if (object == labelObject)
				{
					activator = activatorObj;
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

		if (hitColGrp == nullptr)
		{
			const std::unordered_set<CndHealthDec*> currentConditions(registeredHullConditions);
			for (const auto& condition : currentConditions)
			{
				if (registeredHullConditions.contains(condition) && condition->Matches(damagedObject, incomingDamage, damageList, hitColGrp))
					condition->ExecuteTrigger();
			}
		}
		else
		{
			const std::unordered_set<CndHealthDec*> currentConditions(registeredColGrpConditions);
			for (const auto& condition : currentConditions)
			{
				if (registeredColGrpConditions.contains(condition) && condition->Matches(damagedObject, incomingDamage, damageList, hitColGrp))
					condition->ExecuteTrigger();
			}
		}
	}

	namespace Hooks
	{
		namespace CndHealthDec
		{
			void __stdcall ShipColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, hitColGrp);
			}

			void __stdcall ShipHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, nullptr);
			}
		}
	}
}