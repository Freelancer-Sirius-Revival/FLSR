#include "ExplosionDamage.h"
#include "Plugin.h"
#include "FLCoreDACom.h"
#include <algorithm>

namespace ExplosionDamage
{
	const uint rootCrc = DACOM_CRC::GetCRC32("root");

	static float SquaredDistance3D(const Vector& v1, const Vector& v2, const float radius)
	{
		const float sq1 = v1.x - v2.x, sq2 = v1.y - v2.y, sq3 = v1.z - v2.z;
		return (sq1 * sq1 + sq2 * sq2 + sq3 * sq3 - radius * radius);
	}

	static float CalculateDistanceToShipSurface(const IObjRW* iobj, const Vector& explosionPosition)
	{
		float physicalRadius;
		Vector physicalCenter;
		iobj->get_physical_radius(physicalRadius, physicalCenter);
		float squaredDistance = -1.0f;
		PhySys::RayHit rayHits[16];
		const int collisionCount = PhySys::FindRayCollisions(iobj->cobj->system, explosionPosition, physicalCenter, rayHits, 16);
		for (size_t index = 0; index < collisionCount; index++)
		{
			if (reinterpret_cast<CSimple*>(rayHits[index].cobj) != iobj->cobj)
				continue;
			const Vector explosionVelocity = { explosionPosition.x - rayHits[index].position.x,
											   explosionPosition.y - rayHits[index].position.y,
											   explosionPosition.z - rayHits[index].position.z };
			squaredDistance = explosionVelocity.x * explosionVelocity.x + explosionVelocity.y * explosionVelocity.y + explosionVelocity.z * explosionVelocity.z;
			break;
		}

		// Fallback in case no ray hit, use the center coordinate of the ship for the distance.
		if (squaredDistance < 0.0f)
			squaredDistance = SquaredDistance3D(physicalCenter, explosionPosition, physicalRadius);

		return sqrtf(squaredDistance);
	}

	static float CalculateDamageFractionByDistance(const float explosionRadius, const float distance)
	{
		if (distance > explosionRadius)
			return 0.0f;

		const float minFullDamageRadius = explosionRadius * 0.1f;
		if (distance <= minFullDamageRadius)
			return 1.0f;
		
		const float minDamageFraction = 0.1f;
		return 1.0f - (1.0f - minDamageFraction) * ((distance - minFullDamageRadius) / (explosionRadius - minFullDamageRadius));
	}

	static bool DealShieldDamage(IObjRW* iobj, const ExplosionDamageEvent* explosion, DamageList* dmg)
	{
		CEShield* shield = reinterpret_cast<CEShield*>(reinterpret_cast<CEqObj*>(iobj->cobj)->equip_manager.FindFirst(Shield));
		if (!shield || shield->currShieldHitPoints <= 0.0f || !shield->IsFunctioning() || !shield->IsActive())
			return false;

		float damage = 0.0f;
		const float maxDamage = explosion->explosionArchetype->fHullDamage * ShieldEquipConsts::HULL_DAMAGE_FACTOR + explosion->explosionArchetype->fEnergyDamage;
		if (maxDamage != 0.0f)
		{
			const float distance = CalculateDistanceToShipSurface(iobj, explosion->explosionPosition);
			damage = maxDamage * CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, distance);
		}
		// Always apply damage, even if it is zero. This is to make sure the other ship registers the hit to possibly react to it.
		iobj->damage_shield_direct(shield, damage, dmg);
		return true;
	}

	static void DealEquipmentDamage(IObjRW* iobj, const ExplosionDamageEvent* explosion, DamageList* dmg)
	{
		if (!(iobj->cobj->objectClass & CObject::CEQOBJ_MASK))
			return;

		CEquipManager& equipManager = reinterpret_cast<CEqObj*>(iobj->cobj)->equip_manager;
		CEquipTraverser traverser(ExternalEquipment);
		CAttachedEquip* equip;
		while (equip = reinterpret_cast<CAttachedEquip*>(equipManager.Traverse(traverser)))
		{
			if (equip->archetype->fExplosionResistance == 0.0f)
				continue;

			Vector centerOfMass;
			float radius;
			equip->GetCenterOfMass(centerOfMass);
			equip->GetRadius(radius);
			const float distance = sqrtf(SquaredDistance3D(centerOfMass, explosion->explosionPosition, radius));
			const float damageFraction = CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, distance);
			if (damageFraction <= 0.0f)
				continue;

			const float damage = explosion->explosionArchetype->fHullDamage * equip->archetype->fExplosionResistance * damageFraction;
			iobj->damage_ext_eq(equip, damage, dmg);
		}
	}

	static void DealHullAndColGrpDamage(IObjRW* iobj, const ExplosionDamageEvent* explosion, DamageList* dmg)
	{
		//long* instances;
		//int foundInstances = 0;
		//CompoundInstanceList((long)iobj->cobj->index, &foundInstances, &instances);
		//for (size_t index = 0; index < foundInstances; index++)
		//{
		//	auto bla= iobj->cobj->inst_to_part(instances[index]);
		//	bla = bla;
		//}

		float minSingleRootDamage = 0.0f;
		float maxSingleRootDamage = 0.0f;
		float damageToHull = 0.0f;

		if (iobj->cobj->archetype->fExplosionResistance != 0.0f)
		{
			const float distance = CalculateDistanceToShipSurface(iobj, explosion->explosionPosition);
			const float damageFraction = CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, distance);
			const float damage = explosion->explosionArchetype->fHullDamage * iobj->cobj->archetype->fExplosionResistance * damageFraction;
			minSingleRootDamage = min(damage, minSingleRootDamage);
			maxSingleRootDamage = max(damage, maxSingleRootDamage);
			damageToHull = damage;
		}

		if (iobj->cobj->objectClass & CObject::CEQOBJ_MASK)
		{
			const CEquipManager& equipManager = reinterpret_cast<CEqObj*>(iobj->cobj)->equip_manager;
			CArchGroupManager& colGrpManager = reinterpret_cast<CEqObj*>(iobj->cobj)->archGroupManager;
			CArchGrpTraverser traverser;
			CArchGroup* colGrp;
			while (colGrp = colGrpManager.Traverse(traverser))
			{
				if (colGrp->colGrp->explosionResistance == 0.0f)
					continue;

				Vector centerOfMass;
				colGrp->GetCenterOfMass(centerOfMass);

				float squaredDistance = -1.0f;
				PhySys::RayHit rayHits[16];
				const int collisionCount = PhySys::FindRayCollisions(iobj->cobj->system, explosion->explosionPosition, centerOfMass, rayHits, 16);
				for (size_t index = 0; index < collisionCount; index++)
				{
					const CSimple* rayHitObj = reinterpret_cast<CSimple*>(rayHits[index].cobj);
					// // Moving ship parts are independent objects in space with COBJECT_MASK and the same archetype. In this case go further for more checks.
					//if (rayHitObj != iobj->cobj && !(rayHitObj->objectClass == CObject::COBJECT_MASK && rayHitObj->archetype == iobj->cobj->archetype))
					//	continue;

					if (rayHitObj != iobj->cobj)
						continue;

					const auto instanceId = iobj->cobj->part_to_inst(rayHits[index].hitPartOrHardpointCrc);

					// Find if the hit object is equipment. If so, skip because we do treat them not as extension of the colGrp.
					const auto subObjId = reinterpret_cast<CEqObj*>(iobj->cobj)->inst_to_subobj_id(instanceId);
					if (equipManager.FindByID(subObjId))
						continue;

					if (!instanceId || !colGrp->IsInstInGroup(instanceId))
						continue;

					const Vector explosionVelocity = { explosion->explosionPosition.x - rayHits[index].position.x,
													   explosion->explosionPosition.y - rayHits[index].position.y,
													   explosion->explosionPosition.z - rayHits[index].position.z };
					squaredDistance = explosionVelocity.x * explosionVelocity.x + explosionVelocity.y * explosionVelocity.y + explosionVelocity.z * explosionVelocity.z;
					break;
				}

				// Fallback in case no ray hit, use the center coordinate of the colGrpy for the distance.
				if (squaredDistance < 0.0f)
					squaredDistance = SquaredDistance3D(centerOfMass, explosion->explosionPosition, 0.0f); // The radius of collision groups often is just 0.0f.

				const float damageFraction = CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, sqrtf(squaredDistance));
				if (damageFraction <= 0.0f)
					continue;

				const float damage = explosion->explosionArchetype->fHullDamage * colGrp->colGrp->explosionResistance * damageFraction;
				iobj->damage_col_grp(colGrp, damage, dmg);

				if (colGrp->IsRootHealthProxy())
				{
					minSingleRootDamage = min(damage, minSingleRootDamage);
					maxSingleRootDamage = max(damage, maxSingleRootDamage);
					damageToHull += damage;
				}
			}
		}

		// 'damage' might be between singular damages if exotic explosion resistances (e.g. negative ones) are used.
		// Limit the actual damage by the lower and upper clamp to be sure there's no damage multiplication done by collision groups. The most single dealt damage sets the bar.
		// Do not skip on applying hull damage. Even at 0 it must be done to inform the ship about being hit by something.
		iobj->damage_hull(std::ranges::clamp(damageToHull, minSingleRootDamage, maxSingleRootDamage), dmg);
	}

	bool __stdcall ExplosionHit(IObjRW* iobj, ExplosionDamageEvent* explosion, DamageList* dmg)
	{
		if (DealShieldDamage(iobj, explosion, dmg))
		{
			returncode = NOFUNCTIONCALL;
			return true;
		}

		if (explosion->explosionArchetype->fHullDamage == 0.0f)
		{
			// Still apply damage to make the ship register being hit by something.
			iobj->damage_hull(0.0f, dmg);
			returncode = NOFUNCTIONCALL;
			return true;
		}

		// Energy damage is being skipped because in FL:SR we don't do energy damage.
		DealEquipmentDamage(iobj, explosion, dmg);
		DealHullAndColGrpDamage(iobj, explosion, dmg);

		returncode = NOFUNCTIONCALL;
		return true;
	}
}