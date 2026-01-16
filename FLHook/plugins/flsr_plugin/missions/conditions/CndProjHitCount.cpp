#include "CndProjHitCount.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndProjHitCount*> observedCndProjHitCount;
	std::vector<CndProjHitCount*> orderedCndProjHitCount;

	CndProjHitCount::CndProjHitCount(const ConditionParent& parent,
							const uint damagedObjNameOrLabel,
							const DamagedSurface targetSurface,
							const DamageType damageType,
							const uint targetHitCount,
							const uint inflictorObjNameOrLabel,
							const bool damagedIsActivator) :
		Condition(parent),
		damagedObjNameOrLabel(damagedObjNameOrLabel),
		targetSurface(targetSurface),
		damageType(damageType),
		targetHitCount(targetHitCount),
		currentHitCount(0),
		inflictorObjNameOrLabel(inflictorObjNameOrLabel),
		damagedIsActivator(damagedIsActivator)
	{}

	CndProjHitCount::~CndProjHitCount()
	{
		Unregister();
	}

	void CndProjHitCount::Register()
	{
		currentHitCount = 0;
		if (observedCndProjHitCount.insert(this).second)
			orderedCndProjHitCount.push_back(this);
	}

	void CndProjHitCount::Unregister()
	{
		observedCndProjHitCount.erase(this);
		if (const auto it = std::find(orderedCndProjHitCount.begin(), orderedCndProjHitCount.end(), this); it != orderedCndProjHitCount.end())
			orderedCndProjHitCount.erase(it);
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

		const MissionObject inflictorObj = damageList->is_inflictor_a_player()
												? MissionObject(MissionObjectType::Client, HkGetClientIDByShip(damageList->get_inflictor_id()))
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
					if (object == inflictorObj)
					{
						inflictorObjMatches = true;
						break;
					}
				}
			}
		}

		if (inflictorObjMatches)
		{
			if (damagedIsActivator)
			{
				activator = damagedObject->is_player()
								? MissionObject(MissionObjectType::Client, damagedObject->cobj->ownerPlayer)
								: MissionObject(MissionObjectType::Object, damagedObject->get_id());
			}
			else
			{
				activator = inflictorObj;
			}
			return true;
		}
		return false;
	}

	static void LoopThroughConditions(const IObjRW* damagedObject, const float incomingDamage, const DamageList* damageList, const CndProjHitCount::DamagedSurface hitSurface)
	{
		// Ignore any damage caused by repair guns.
		if (incomingDamage <= 0.0f)
			return;

		const auto currentConditions(orderedCndProjHitCount);
		for (const auto& condition : currentConditions)
		{
			if (observedCndProjHitCount.contains(condition) && condition->Matches(damagedObject, damageList, hitSurface))
				condition->ExecuteTrigger();
		}
	}

	namespace Hooks
	{
		namespace CndProjHitCount
		{
			void __stdcall ShipAndSolarEquipDamage(const IObjRW* damagedObject, const CEquip* hitEquip, const float& incomingDamage, const DamageList* damageList)
			{
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Hull);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall ShipAndSolarColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList)
			{
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Hull);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall ShipAndSolarHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList)
			{
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Hull);
				returncode = DEFAULT_RETURNCODE;
			}

			void __stdcall ShipAndSolarShieldDamage(const IObjRW* damagedObject, const CEShield* hitShield, const float& incomingDamage, const DamageList* damageList)
			{
				LoopThroughConditions(damagedObject, incomingDamage, damageList, Missions::CndProjHitCount::DamagedSurface::Shield);
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}