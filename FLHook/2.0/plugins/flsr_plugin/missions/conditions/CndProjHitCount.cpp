#include "CndProjHitCount.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndProjHitCount*> registeredConditions;

	CndProjHitCount::CndProjHitCount(const ConditionParent& parent,
							const uint damagedObjNameOrLabel,
							const DamagedSurface targetSurface,
							const DamageType damageType,
							const uint targetHitCount,
							const uint inflictorObjNameOrLabel) :
		Condition(parent),
		damagedObjNameOrLabel(damagedObjNameOrLabel),
		targetSurface(targetSurface),
		damageType(damageType),
		targetHitCount(targetHitCount),
		currentHitCount(0),
		inflictorObjNameOrLabel(inflictorObjNameOrLabel)
	{}

	CndProjHitCount::~CndProjHitCount()
	{
		Unregister();
	}

	void CndProjHitCount::Register()
	{
		currentHitCount = 0;
		registeredConditions.insert(this);
	}

	void CndProjHitCount::Unregister()
	{
		registeredConditions.erase(this);
	}

	bool CndProjHitCount::Matches(const IObjRW* damagedObject, const DamageList* damageList, const DamagedSurface damagedSurface)
	{
		if (targetSurface != DamagedSurface::Any && targetSurface != damagedSurface)
			return false;

		bool damageTypeMatches = false;
		const auto& damageCause = damageList->get_cause();
		switch (damageType)
		{
			case DamageType::Any:
			{
				damageTypeMatches = damageCause == DamageCause::Gun ||
									damageCause == DamageCause::MissileTorpedo ||
									damageCause == DamageCause::CruiseDisrupter ||
									damageCause == DamageCause::Mine ||
									damageCause == DamageCause::DummyDisrupter ||
									damageCause == DamageCause::UnkDisrupter;
				break;
			}

			case DamageType::Projectile:
			{
				damageTypeMatches = damageCause == DamageCause::Gun;
				break;
			}

			case DamageType::Explosion:
			{
				damageTypeMatches = damageCause == DamageCause::MissileTorpedo ||
									damageCause == DamageCause::CruiseDisrupter ||
									damageCause == DamageCause::Mine ||
									damageCause == DamageCause::DummyDisrupter ||
									damageCause == DamageCause::UnkDisrupter;
				break;
			}
		}
		if (!damageTypeMatches)
			return false;

		const auto& mission = missions.at(parent.missionId);
		bool damagedObjMatches = mission.objectIds.contains(damagedObject->cobj->id);
		if (!damagedObjMatches)
		{
			if (const auto& objectsByLabel = mission.objectsByLabel.find(damagedObjNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.id == (object.type == MissionObjectType::Client ? damagedObject->cobj->ownerPlayer : damagedObject->cobj->id))
					{
						damagedObjMatches = true;
						break;
					}
				}
			}
		}

		if (!damagedObjMatches)
			return false;

		const MissionObject activatorObj = damageList->is_inflictor_a_player()
			? MissionObject(MissionObjectType::Client, damageList->get_inflictor_owner_player())
			: MissionObject(MissionObjectType::Object, damageList->get_inflictor_id());

		bool inflictorObjMatches = true;
		if (inflictorObjNameOrLabel != 0)
		{
			inflictorObjMatches = false;

			if (inflictorObjNameOrLabel == Stranger)
			{
				inflictorObjMatches = (damageList->is_inflictor_a_player() && !mission.clientIds.contains(damageList->get_inflictor_owner_player())) || !mission.objectIds.contains(damageList->get_inflictor_id());
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(inflictorObjNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object == activatorObj)
					{
						inflictorObjMatches = true;
						break;
					}
				}
			}
		}

		if (inflictorObjMatches)
		{
			activator = activatorObj;
			return true;
		}
		return false;
	}

	static void LoopThroughConditions(const IObjRW* damagedObject, const float incomingDamage, const DamageList* damageList, const CndProjHitCount::DamagedSurface hitSurface)
	{
		// Ignore any damage caused by repair guns.
		if (incomingDamage <= 0.0f)
			return;

		const std::unordered_set<CndProjHitCount*> currentConditions(registeredConditions);
		for (const auto& condition : currentConditions)
		{
			if (registeredConditions.contains(condition) && condition->Matches(damagedObject, damageList, hitSurface))
				condition->ExecuteTrigger();
		}
	}

	namespace Hooks
	{
		namespace CndProjHitCount
		{
			void __stdcall ShipEquipDamage(const IObjRW* damagedObject, const CEquip* hitEquip, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Hull);
			}

			void __stdcall ShipColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Hull);
			}

			void __stdcall ShipHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Hull);
			}

			void __stdcall ShipShieldDamage(const IObjRW* damagedObject, const CEShield* hitShield, const float& incomingDamage, const DamageList* damageList)
			{
				returncode = DEFAULT_RETURNCODE;
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Shield);
			}
		}
	}
}