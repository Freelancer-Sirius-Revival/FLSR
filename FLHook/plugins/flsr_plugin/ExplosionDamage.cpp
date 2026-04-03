#include "ExplosionDamage.h"
#include "Plugin.h"
#include <FLCoreDACom.h>
#include <algorithm>

namespace ExplosionDamage
{
	static float SquaredDistance3D(const Vector& v1, const Vector& v2, const float radius)
	{
		const float sq1 = v1.x - v2.x, sq2 = v1.y - v2.y, sq3 = v1.z - v2.z;
		return (sq1 * sq1 + sq2 * sq2 + sq3 * sq3 - radius * radius);
	}

	static bool IsChildOfObject(const CObject* rootObject, const CArchGroup* colGrp, const CObject* otherObject)
	{
		// Animated ship parts are independent objects in space with COBJECT_MASK and the same archetype. In this case go further for more checks.
		if (otherObject != rootObject && otherObject->objectClass == CObject::COBJECT_MASK && otherObject->archetype == rootObject->archetype)
		{
			ObjectTreeNode* node = otherObject->objNode;
			EngineObject* lastObject = nullptr;
			bool childOfColGrp = false;
			while (node != nullptr)
			{
				if (node->type == ObjectTreeNodeType::Object)
					lastObject = node->obj;
				else if (node->type == ObjectTreeNodeType::Virtual)
					childOfColGrp = !colGrp || node->obj == colGrp->objNode->obj;
				node = node->parent;
			}

			return lastObject == rootObject && (colGrp == nullptr || childOfColGrp);
		}
		return false;
	}

	static float CalculateDamageFractionByDistance(const float explosionRadius, const float distance)
	{
		if (distance >= explosionRadius)
			return 0.0f;

		const float minFullDamageRadius = explosionRadius * 0.1f;
		if (distance <= minFullDamageRadius)
			return 1.0f;

		const float minDamageFraction = 0.1f;
		return 1.0f - (1.0f - minDamageFraction) * ((distance - minFullDamageRadius) / (explosionRadius - minFullDamageRadius));
	}

	const Vector dodecahedronNormalizedVectors[] = {
		{  0.0f,		 0.52573111f,	 0.85065081f },
		{  0.0f,		-0.52573111f,	 0.85065081f },
		{  0.0f,		 0.52573111f,	-0.85065081f },
		{  0.0f,		-0.52573111f,	-0.85065081f },
		{  0.85065081f,	 0.0f,			 0.52573111f },
		{ -0.85065081f,	 0.0f,			 0.52573111f },
		{  0.85065081f,	 0.0f,			-0.52573111f },
		{ -0.85065081f,	 0.0f,			-0.52573111f },
		{  0.52573111f,	 0.85065081f,	 0.0f },
		{ -0.52573111f,	 0.85065081f,	 0.0f },
		{  0.52573111f,	-0.85065081f,	 0.0f },
		{ -0.52573111f,	-0.85065081f,	 0.0f }
	};

	const uint RootId = DACOM_CRC::GetCRC32("root");

	static float GetSquaredDistanceFromShipHits(const PhySys::RayHit* rayHits, const size_t entryCount, const Vector& explosionPosition, const CObject* object, const bool rootOnly)
	{
		for (size_t index = 0; index < entryCount; index++)
		{
			if (reinterpret_cast<CObject*>(rayHits[index].cobj) != object && !IsChildOfObject(object, nullptr, reinterpret_cast<CObject*>(rayHits[index].cobj)))
				continue;

			// If we only want root, it must not be the part of a col grp (explicit or implicit) and no equipment.
			if (rootOnly && (object->objectClass & CObject::CEQOBJ_MASK))
			{
				const CEqObj* eqObject = (CEqObj*)object;
				// If the hit object isn't root itself, try to see if it is attached to any colGrp.
				if (rayHits[index].hitPartOrHardpointCrc != RootId)
				{
					const auto instanceId = object->part_to_inst(rayHits[index].hitPartOrHardpointCrc);
					// Find if the hit object is equipment. If so, skip because we do treat them not as extension of the hull here.
					const auto subObjId = eqObject->inst_to_subobj_id(instanceId);
					if (eqObject->equip_manager.FindByID(subObjId))
						continue;

					CArchGroupManager& colGrpManager = ((CEqObj*)object)->archGroupManager;
					CArchGrpTraverser traverser;
					CArchGroup* colGrp = nullptr;
					const auto& otherObject = reinterpret_cast<CObject*>(rayHits[index].cobj);
					bool partOfColGrp = false;
					while (colGrp = colGrpManager.Traverse(traverser))
					{
						// Either the hit object is a collision group, or it is the child of one.
						if ((instanceId && colGrp->IsInstInGroup(instanceId)) || IsChildOfObject(object, colGrp, otherObject))
						{
							partOfColGrp = true;
							break;
						}
					}
					// The hit object must not be a col grp or part of it.
					if (partOfColGrp)
						continue;
				}
			}
			const Vector distanceVec = { explosionPosition.x - rayHits[index].position.x,
										 explosionPosition.y - rayHits[index].position.y,
										 explosionPosition.z - rayHits[index].position.z };
			return distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y + distanceVec.z * distanceVec.z;
		}
		return FLT_MAX;
	}

	static float FindClosestDistanceToShipSurface(const CObject* object, const Vector& explosionPosition, const float explosionRadius, const bool rootOnly)
	{
		float closestDistanceSquared = explosionRadius * explosionRadius;
		const Vector& physicalCenter = object->get_center_of_mass();

		const byte maxRayCasts = 32;
		PhySys::RayHit rayHits[maxRayCasts];
		int rayCasts = 1;
		if (rootOnly && object->objectClass & CObject::CEQOBJ_MASK)
		{
			const auto colGrps = ((CEqObj*)object)->archGroupManager.size;
			rayCasts = std::min<char>(maxRayCasts, colGrps + 4); // +4 for possible hardpoints in the way
		}

		int rayHitCount = PhySys::FindRayCollisions(object->system, explosionPosition, physicalCenter, rayHits, rayCasts);
		closestDistanceSquared = std::min<float>(closestDistanceSquared, GetSquaredDistanceFromShipHits(rayHits, rayHitCount, explosionPosition, object, rootOnly));
		
		for (const Vector& normalizedVec : dodecahedronNormalizedVectors)
		{
			const Vector targetVector = {
				normalizedVec.x * explosionRadius + explosionPosition.x,
				normalizedVec.y * explosionRadius + explosionPosition.y,
				normalizedVec.z * explosionRadius + explosionPosition.z
			};
			rayHitCount = PhySys::FindRayCollisions(object->system, explosionPosition, targetVector, rayHits, rayCasts);
			closestDistanceSquared = std::min<float>(closestDistanceSquared, GetSquaredDistanceFromShipHits(rayHits, rayHitCount, explosionPosition, object, rootOnly));
		}
		return sqrtf(closestDistanceSquared);
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
			const float distance = FindClosestDistanceToShipSurface(iobj->cobj, explosion->explosionPosition, explosion->explosionArchetype->fRadius, false);
			damage = maxDamage * CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, distance);
		}
		// Always apply damage to make sure the object gets notified about the hit.
		iobj->damage_shield_direct(shield, damage, dmg);
		ConPrint(L"Shield: " + std::to_wstring(damage) + L"\n");
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

	static float GetSquaredDistanceFromColGrpHits(const PhySys::RayHit* rayHits, const size_t entryCount, const Vector& explosionPosition, const CEqObj* object, const CArchGroup* colGrp)
	{
		for (size_t index = 0; index < entryCount; index++)
		{
			if (reinterpret_cast<CObject*>(rayHits[index].cobj) == object)
			{
				const auto instanceId = object->part_to_inst(rayHits[index].hitPartOrHardpointCrc);
				// Find if the hit object is equipment. If so, skip because we do treat them not as extension of the colGrp.
				const auto subObjId = object->inst_to_subobj_id(instanceId);
				if (object->equip_manager.FindByID(subObjId))
					continue;
				if (!instanceId || !colGrp->IsInstInGroup(instanceId))
					continue;
			}
			// Animated ship parts are independent objects in space.
			else if (!IsChildOfObject(object, colGrp, reinterpret_cast<CObject*>(rayHits[index].cobj)))
				continue;

			const Vector distanceVec = { explosionPosition.x - rayHits[index].position.x,
										 explosionPosition.y - rayHits[index].position.y,
										 explosionPosition.z - rayHits[index].position.z };
			return distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y + distanceVec.z * distanceVec.z;
		}
		return FLT_MAX;
	}

	static std::pair<float, float> FindClosestDistanceToColGrpSurface(const CEqObj* object, const CArchGroup* colGrp, const Vector& explosionPosition, const float explosionRadius)
	{
		const CArchGroupManager& colGrpManager = object->archGroupManager;
		float closestColGrpDistanceSquared = explosionRadius * explosionRadius;
		// This also collects the distance to root surfaces in case they are far off the actual center of the ship (e.g. Rheinland Battleship)
		float closestRootDistanceSquared = closestColGrpDistanceSquared;
		const bool collectClosestRootDistance = object->archetype->fExplosionResistance != 0.0f;
		Vector physicalCenter;
		colGrp->GetCenterOfMass(physicalCenter);
		const byte maxRayCasts = 32;
		PhySys::RayHit rayHits[maxRayCasts];
		const int rayCasts = std::min<char>(maxRayCasts, colGrpManager.size + 4); // +4 for possible hardpoints in the way

		int rayHitCount = PhySys::FindRayCollisions(object->system, explosionPosition, physicalCenter, rayHits, rayCasts);
		closestColGrpDistanceSquared = std::min<float>(closestColGrpDistanceSquared, GetSquaredDistanceFromColGrpHits(rayHits, rayHitCount, explosionPosition, object, colGrp));
		if (collectClosestRootDistance)
			closestRootDistanceSquared = std::min<float>(closestRootDistanceSquared, GetSquaredDistanceFromShipHits(rayHits, rayHitCount, explosionPosition, object, true));

		for (const Vector& normalizedVec : dodecahedronNormalizedVectors)
		{
			const Vector targetVector = {
				normalizedVec.x * explosionRadius + explosionPosition.x,
				normalizedVec.y * explosionRadius + explosionPosition.y,
				normalizedVec.z * explosionRadius + explosionPosition.z
			};
			rayHitCount = PhySys::FindRayCollisions(object->system, explosionPosition, targetVector, rayHits, rayCasts);
			closestColGrpDistanceSquared = std::min<float>(closestColGrpDistanceSquared, GetSquaredDistanceFromColGrpHits(rayHits, rayHitCount, explosionPosition, object, colGrp));
			if (collectClosestRootDistance)
				closestRootDistanceSquared = std::min<float>(closestRootDistanceSquared, GetSquaredDistanceFromShipHits(rayHits, rayHitCount, explosionPosition, object, true));
		}
		return { sqrtf(closestColGrpDistanceSquared), sqrtf(closestRootDistanceSquared) };
	}

	static void DealHullAndColGrpDamage(IObjRW* iobj, const ExplosionDamageEvent* explosion, DamageList* dmg)
	{
		const float explosionRadius = explosion->explosionArchetype->fRadius;
		const Vector& explosionPosition = explosion->explosionPosition;
		if (!(iobj->cobj->objectClass & CObject::CEQOBJ_MASK))
		{
			if (iobj->cobj->archetype->fExplosionResistance == 0.0f)
			{
				// Always deal damage to notify the object about being hit
				iobj->damage_hull(0.0f, dmg);
				return;
			}
			const float distance = FindClosestDistanceToShipSurface(iobj->cobj, explosionPosition, explosionRadius, false);
			const float damageFraction = CalculateDamageFractionByDistance(explosionRadius, distance);
			const float damage = explosion->explosionArchetype->fHullDamage * iobj->cobj->archetype->fExplosionResistance * damageFraction;
			iobj->damage_hull(damage, dmg);
			return;
		}

		float minSingleRootDamage = 0.0f; // used for negative damage (possibly also multiplied by explosion resistance)
		float maxSingleRootDamage = 0.0f; // used for positive damage (possibly also multiplied by explosion resistance)
		float damageToRoot = 0.0f; // accumulated damage to root
		float closestRootDistance = explosionRadius;
		const CEqObj* object = reinterpret_cast<CEqObj*>(iobj->cobj);

		if (object->archetype->fExplosionResistance != 0.0f)
		{
			closestRootDistance = FindClosestDistanceToShipSurface(object, explosion->explosionPosition, explosion->explosionArchetype->fRadius, true);
			const float damageFraction = CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, closestRootDistance);
			const float damage = explosion->explosionArchetype->fHullDamage * object->archetype->fExplosionResistance * damageFraction;
			minSingleRootDamage = damage;
			maxSingleRootDamage = damage;
			damageToRoot = damage;
		}

		CArchGroupManager& colGrpManager = reinterpret_cast<CEqObj*>(iobj->cobj)->archGroupManager;
		CArchGrpTraverser traverser;
		CArchGroup* colGrp = nullptr;
		while (colGrp = colGrpManager.Traverse(traverser))
		{
			if (colGrp->colGrp->explosionResistance == 0.0f)
				continue;

			const auto distances = FindClosestDistanceToColGrpSurface(object, colGrp, explosionPosition, explosionRadius);
			// This also collects the distance to root surfaces in case they are far off the actual center of the ship (e.g. Rheinland Battleship)
			closestRootDistance = std::min<float>(closestRootDistance, distances.second);

			const float damageFraction = CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, distances.first);
			if (damageFraction <= 0.0f)
				continue;
			const float damage = explosion->explosionArchetype->fHullDamage * colGrp->colGrp->explosionResistance * damageFraction;
			iobj->damage_col_grp(colGrp, damage, dmg);
			ConPrint(stows(colGrp->colGrp->name.value) + L": " + std::to_wstring(damage) + L"\n");

			if (colGrp->IsRootHealthProxy())
			{
				minSingleRootDamage = std::min<float>(damage, minSingleRootDamage);
				maxSingleRootDamage = std::max<float>(damage, maxSingleRootDamage);
				damageToRoot += damage;
			}
		}

		// 'damage' might be between singular damages if exotic explosion resistances (e.g. negative ones) are used.
		// Limit the actual damage by the lower and upper clamp to be sure there's no damage multiplication done by collision groups. The most single dealt damage sets the bar.
		// Do not skip on applying hull damage. Even at 0 it must be done to inform the ship about being hit by something.
		iobj->damage_hull(std::ranges::clamp(damageToRoot, minSingleRootDamage, maxSingleRootDamage), dmg);
		ConPrint(L"Root: " + std::to_wstring(std::ranges::clamp(damageToRoot, minSingleRootDamage, maxSingleRootDamage)) + L"\n");
	}

	bool __stdcall ExplosionHit(IObjRW* iobj, ExplosionDamageEvent* explosion, DamageList* dmg)
	{
		ConPrint(L"\nShip: " + std::to_wstring(iobj->cobj->archetype->iArchID) + L"\n");
		ConPrint(L"Max Explosion HullDmg: " + std::to_wstring(explosion->explosionArchetype->fHullDamage) + L"\n");
		ConPrint(L"Explosion Radius: " + std::to_wstring(explosion->explosionArchetype->fRadius) + L"\n");
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