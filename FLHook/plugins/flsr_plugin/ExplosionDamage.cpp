#include "ExplosionDamage.h"
#include "Plugin.h"
#include <algorithm>
#include <queue>

namespace ExplosionDamage
{
	static std::unordered_set<CObject*> FindAllRelatedObjects(const CObject* object)
	{
		std::unordered_set<CObject*> result;
		std::queue<ObjectTreeNode*> nodes;
		nodes.push(object->objNode);
		while (!nodes.empty())
		{
			const ObjectTreeNode* objNode = nodes.front();
			nodes.pop();
			if (!objNode)
				continue;
			if (objNode->type == ObjectTreeNodeType::Object && objNode->obj && objNode->obj->objectClass != CObject::CEQUIPMENT_OBJECT)
				result.insert(objNode->obj);
			if (objNode->firstChild)
				nodes.push(objNode->firstChild);
			if (objNode->nextSibling)
				nodes.push(objNode->nextSibling);
		}
		return result;
	}

	static size_t CountAllNodes(const CObject* object)
	{
		size_t result = 0;
		std::queue<ObjectTreeNode*> nodes;
		nodes.push(object->objNode);
		while (!nodes.empty())
		{
			result++;
			const ObjectTreeNode* objNode = nodes.front();
			nodes.pop();
			if (!objNode)
				continue;
			if (objNode->firstChild)
				nodes.push(objNode->firstChild);
			if (objNode->nextSibling)
				nodes.push(objNode->nextSibling);
		}
		return result;
	}

	static float CalculateDamageFractionByDistance(const float explosionRadius, const float minFullDamageDistance, const float distance)
	{
		if (distance >= explosionRadius)
			return 0.0f;

		if (distance <= minFullDamageDistance)
			return 1.0f;

		if (minFullDamageDistance >= explosionRadius)
			return 1.0f;

		return 1.0f - ((distance - minFullDamageDistance) / (explosionRadius - minFullDamageDistance));
	}

	static float FindDetonationDistance(const uint projectileId)
	{
		CProjectile* projectile = static_cast<CProjectile*>(CObject::Find(projectileId, CObject::CGUIDED_OBJECT));
		if (projectile)
			return static_cast<Archetype::Munition*>(projectile->archetype)->fDetonationDist;

		projectile = static_cast<CProjectile*>(CObject::Find(projectileId, CObject::CMINE_OBJECT));
		if (projectile)
			return static_cast<Archetype::Mine*>(projectile->archetype)->fDetonationDist;

		CAsteroid* asteroid = static_cast<CAsteroid*>(CObject::Find(projectileId, CObject::CASTEROID_OBJECT));
		if (asteroid)
			return static_cast<Archetype::Asteroid*>(asteroid->archetype)->fDetectRadius - static_cast<Archetype::Asteroid*>(asteroid->archetype)->fExplosionOffset;

		return 0.0f;
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

	static std::vector<Vector> ComputeSpreadVectors(const Vector& origin, const float length)
	{
		std::vector<Vector> result(std::size(dodecahedronNormalizedVectors));
		for (size_t index = 0, entries = result.size(); index < entries; index++)
		{
			result[index] = Vector({
				dodecahedronNormalizedVectors[index].x * length + origin.x,
				dodecahedronNormalizedVectors[index].y * length + origin.y,
				dodecahedronNormalizedVectors[index].z * length + origin.z
			});
		}
		return result;
	}

	static float GetSquaredDistanceFromHit(const PhySys::RayHit& rayHit, const Vector& explosionPosition)
	{
		const Vector distanceVec = { explosionPosition.x - rayHit.position.x,
									 explosionPosition.y - rayHit.position.y,
									 explosionPosition.z - rayHit.position.z };
		return distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y + distanceVec.z * distanceVec.z;
	}

	static float FindClosestSquareDistanceToObjectSurface(CObject* object, const Vector& explosionPosition, const std::vector<Vector>& explosionSpreadTargets)
	{
		float closestDistanceSquared = FLT_MAX;
		PhySys::RayHit rayHit;
		if (PhySys::FindRayIntersect(object, explosionPosition, object->get_center_of_mass(), &rayHit, 1))
			closestDistanceSquared = std::min<float>(closestDistanceSquared, GetSquaredDistanceFromHit(rayHit, explosionPosition));
		for (const Vector& targetVec : explosionSpreadTargets)
		{
			if (PhySys::FindRayIntersect(object, explosionPosition, targetVec, &rayHit, 1))
				closestDistanceSquared = std::min<float>(closestDistanceSquared, GetSquaredDistanceFromHit(rayHit, explosionPosition));
		}
		return closestDistanceSquared;
	}

	static bool DealShieldDamage(IObjRW* iobj, const ExplosionDamageEvent* explosion, const float minFullDamageDistance, DamageList* dmg)
	{
		CEShield* shield = reinterpret_cast<CEShield*>(reinterpret_cast<CEqObj*>(iobj->cobj)->equip_manager.FindFirst(Shield));
		if (!shield || shield->currShieldHitPoints <= 0.0f || !shield->IsFunctioning() || !shield->IsActive())
			return false;

		float damage = 0.0f;
		const float maxDamage = explosion->explosionArchetype->fHullDamage * ShieldEquipConsts::HULL_DAMAGE_FACTOR + explosion->explosionArchetype->fEnergyDamage;
		if (maxDamage != 0.0f)
		{
			const auto& explosionSpreadTargets = ComputeSpreadVectors(explosion->explosionPosition, explosion->explosionArchetype->fRadius);
			float closestDistanceSquared = FLT_MAX;
			for (const auto& obj : FindAllRelatedObjects(iobj->cobj))
				closestDistanceSquared = std::min<float>(closestDistanceSquared, FindClosestSquareDistanceToObjectSurface(iobj->cobj, explosion->explosionPosition, explosionSpreadTargets));
			damage = maxDamage * CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, minFullDamageDistance, closestDistanceSquared != FLT_MAX ? sqrtf(closestDistanceSquared) : FLT_MAX);
			if (damage != 0.0f)
				iobj->damage_shield_direct(shield, damage, dmg);
		}
		return true;
	}

	static float SquaredDistance3D(const Vector& v1, const Vector& v2, const float radius)
	{
		const float sq1 = v1.x - v2.x, sq2 = v1.y - v2.y, sq3 = v1.z - v2.z;
		return (sq1 * sq1 + sq2 * sq2 + sq3 * sq3 - radius * radius);
	}

	static void DealEquipmentDamage(IObjRW* iobj, const ExplosionDamageEvent* explosion, const float minFullDamageDistance, DamageList* dmg)
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
			const float damageFraction = CalculateDamageFractionByDistance(explosion->explosionArchetype->fRadius, minFullDamageDistance, distance);
			const float damage = damageFraction <= 0.0f ? 0.0f : explosion->explosionArchetype->fHullDamage * equip->archetype->fExplosionResistance * damageFraction;
			if (damage != 0.0f)
				iobj->damage_ext_eq(equip, damage, dmg);
		}
	}

	static void FindClosestExplosionSpreadHits(CObject* object, const size_t rayCasts, const Vector& explosionPosition, const std::vector<Vector>& explosionSpreadTargets, std::unordered_map<uint, std::pair<float, uint>>& results)
	{
		std::vector<PhySys::RayHit> rayHits(rayCasts);
		for (const Vector& targetVec : explosionSpreadTargets)
		{
			int rayHitCount = PhySys::FindRayIntersect(object, explosionPosition, targetVec, rayHits.data(), rayHits.size());
			for (int index = 0; index < rayHitCount; index++)
			{
				auto entry = results.find(rayHits[index].hitPartOrHardpointCrc);
				if (entry == results.end())
					results.insert({ rayHits[index].hitPartOrHardpointCrc, { GetSquaredDistanceFromHit(rayHits[index], explosionPosition), rayHits[index].cobj->part_to_inst(rayHits[index].hitPartOrHardpointCrc) }});
				else
					entry->second.first = std::min<float>(entry->second.first, GetSquaredDistanceFromHit(rayHits[index], explosionPosition));
			}
		}
	}

	static void FindClosestSquareDistancesTowardObjectSurface(CObject* object, const size_t rayCasts, const Vector& targetPosition, const Vector& explosionPosition, std::unordered_map<uint, std::pair<float, uint>>& results)
	{
		std::vector<PhySys::RayHit> rayHits(rayCasts);
		int rayHitCount = PhySys::FindRayIntersect(object, explosionPosition, targetPosition, rayHits.data(), rayHits.size());
		for (int index = 0; index < rayHitCount; index++)
		{
			auto entry = results.find(rayHits[index].hitPartOrHardpointCrc);
			if (entry == results.end())
				results.insert({ rayHits[index].hitPartOrHardpointCrc, { GetSquaredDistanceFromHit(rayHits[index], explosionPosition), rayHits[index].cobj->part_to_inst(rayHits[index].hitPartOrHardpointCrc) } });
			else
				entry->second.first = std::min<float>(entry->second.first, GetSquaredDistanceFromHit(rayHits[index], explosionPosition));
		}
	}

	static void DealHullAndColGrpDamage(IObjRW* iobj, const ExplosionDamageEvent* explosion, const float minFullDamageDistance, DamageList* dmg)
	{
		const float explosionRadius = explosion->explosionArchetype->fRadius;
		const Vector& explosionPosition = explosion->explosionPosition;
		const auto& explosionSpreadTargets = ComputeSpreadVectors(explosion->explosionPosition, explosionRadius);

		// For all simple objects take the simplest check.
		if (!(iobj->cobj->objectClass & CObject::CEQOBJ_MASK))
		{
			const float maxDamage = explosion->explosionArchetype->fHullDamage * iobj->cobj->archetype->fExplosionResistance;
			if (maxDamage == 0.0f)
				return;

			float closestDistanceSquared = FLT_MAX;
			for (const auto& obj : FindAllRelatedObjects(iobj->cobj))
				closestDistanceSquared = std::min<float>(closestDistanceSquared, FindClosestSquareDistanceToObjectSurface(iobj->cobj, explosion->explosionPosition, explosionSpreadTargets));
			const float damageFraction = CalculateDamageFractionByDistance(explosionRadius, minFullDamageDistance, closestDistanceSquared != FLT_MAX ? sqrtf(closestDistanceSquared) : FLT_MAX);
			const float damage = damageFraction <= 0.0f ? 0.0f : maxDamage * damageFraction;
			if (damage != 0.0f)
				iobj->damage_hull(damage, dmg);
			return;
		}

		const CEqObj* mainObject = reinterpret_cast<CEqObj*>(iobj->cobj);
		if (mainObject->objectClass & CObject::CSOLAR_OBJECT)
		{
			const auto& solar = (CSolar*)mainObject;
			if (!solar->isDestructible || (solar->isDynamic && !((Archetype::Solar*)solar->archetype)->bDestructible))
				return;
		}

		// For all objects with equipment/multipart, go for the full set of checks.

		const auto& relatedObjects = FindAllRelatedObjects(mainObject);
		std::unordered_map<uint, std::pair<float, uint>> squareDistanceAndInstanceIdByCrc;
		std::unordered_map<CObject*, size_t> nodeCountPerObject;

		// Find all distances along the way to the object centers.
		for (const auto& obj : relatedObjects)
		{
			nodeCountPerObject.insert({ obj, CountAllNodes(obj) });
			FindClosestExplosionSpreadHits(obj, nodeCountPerObject.at(obj), explosionPosition, explosionSpreadTargets, squareDistanceAndInstanceIdByCrc);
			FindClosestSquareDistancesTowardObjectSurface(obj, nodeCountPerObject.at(obj), obj->vPos /* CenterOfMass contains sometimes no, sometimes "different" coordinates. */, explosionPosition, squareDistanceAndInstanceIdByCrc);
		}

		// Find all distances along the way to the collision group centers.
		CArchGroupManager& colGrpManager = reinterpret_cast<CEqObj*>(iobj->cobj)->archGroupManager;
		{
			CArchGrpTraverser traverser;
			CArchGroup* colGrp = nullptr;
			while (colGrp = colGrpManager.Traverse(traverser))
			{
				if (colGrp->fate != 0 || colGrp->colGrp->explosionResistance == 0.0f || (uint)colGrp->objNode == 0xffffffff /* Col grp is wrongly defined in INI and does not exist on the model. */)
					continue;

				Vector colGrpCenter;
				colGrp->GetCenterOfMass(colGrpCenter); // This value isn't always very reliable. E.g. the Liberty Freighter's side groups are "centered" inside the ship's main hull.
				for (const auto& obj : relatedObjects)
				{
					// Only if this colGrp is a direct child of the current obj.
					if (colGrp->objNode->obj == obj)
						FindClosestSquareDistancesTowardObjectSurface(obj, nodeCountPerObject.at(obj), colGrpCenter, explosionPosition, squareDistanceAndInstanceIdByCrc);
				}
			}
		}

		// Delete all non-relevant entries.
		const float explosionRadiusSquared = explosionRadius * explosionRadius;
		for (auto it = squareDistanceAndInstanceIdByCrc.begin(); it != squareDistanceAndInstanceIdByCrc.end();)
		{
			if (it->second.first >= explosionRadiusSquared)
				it = squareDistanceAndInstanceIdByCrc.erase(it);
			else if (const auto& subObjId = mainObject->inst_to_subobj_id(it->second.second); mainObject->equip_manager.FindByID(subObjId))
				it = squareDistanceAndInstanceIdByCrc.erase(it);
			else
				it++;
		}

		// Collect individual parts and their distances into actual collision groups with the shortest distance.
		std::unordered_map<CArchGroup*, float> distanceSquaredByColGrp;
		for (const auto& entry : squareDistanceAndInstanceIdByCrc)
		{
			// A part instance can be member of multiple ColGrps if these are children of each other.
			std::unordered_set<CArchGroup*> containingArchGrps;
			{
				CArchGrpTraverser traverser;
				CArchGroup* colGrp = nullptr;
				while (colGrp = colGrpManager.Traverse(traverser))
				{
					if (colGrp->IsInstInGroup(entry.second.second))
						containingArchGrps.insert(colGrp);
				}
			}

			// Find the ColGrp that's the most far away from Root in the dependency tree. This way we always find the "leaf" this part instance should really be the part of.
			std::pair<CArchGroup*, size_t> mostDistancedColGrp = { nullptr, 0 };
			if (!containingArchGrps.empty())
			{
				for (const auto& colGrp : containingArchGrps)
				{
					size_t iterations = 0;
					ObjectTreeNode* objNode = colGrp->objNode;
					while (objNode)
					{
						iterations++;
						objNode = objNode->parent;
					}
					if (iterations > mostDistancedColGrp.second)
					{
						mostDistancedColGrp.first = colGrp;
						mostDistancedColGrp.second = iterations;
					}
				}
			}
			// If the found colGrp is NULL, it basically means Root.
			auto foundEntry = distanceSquaredByColGrp.find(mostDistancedColGrp.first);
			if (foundEntry == distanceSquaredByColGrp.end())
				distanceSquaredByColGrp.insert({ mostDistancedColGrp.first, entry.second.first });
			else
				foundEntry->second = std::min<float>(foundEntry->second, entry.second.first);
		}

		// Apply the damage.
		float minSingleRootDamage = 0.0f; // used for negative damage (possibly also multiplied by explosion resistance)
		float maxSingleRootDamage = 0.0f; // used for positive damage (possibly also multiplied by explosion resistance)
		float damageToRoot = 0.0f; // accumulated damage to root
		
		for (const auto& [ colGrp, distanceSquared ] : distanceSquaredByColGrp)
		{
			const float maxDamage = explosion->explosionArchetype->fHullDamage * (colGrp ? colGrp->colGrp->explosionResistance : mainObject->archetype->fExplosionResistance);
			if (maxDamage == 0.0f)
				continue;

			const float damageFraction = CalculateDamageFractionByDistance(explosionRadius, minFullDamageDistance, sqrtf(distanceSquared));
			const float damage = damageFraction <= 0.0f ? 0.0f : maxDamage * damageFraction;
			if (damage == 0.0f)
				continue;

			if (colGrp)
			{
				CArchGroup* modifiableColGrp = (CArchGroup*)colGrp;
				// Temporarily disable RootHealthProxy to make sure the game doesn't internally do this. We care for that ourselves after.
				const bool originalFlag = modifiableColGrp->colGrp->rootHealthProxy;
				modifiableColGrp->colGrp->rootHealthProxy = false;
				iobj->damage_col_grp(modifiableColGrp, damage, dmg);
				modifiableColGrp->colGrp->rootHealthProxy = originalFlag;
			}

			if (!colGrp || colGrp->IsRootHealthProxy())
			{
				minSingleRootDamage = std::min<float>(damage, minSingleRootDamage);
				maxSingleRootDamage = std::max<float>(damage, maxSingleRootDamage);
				damageToRoot += damage;
			}
		}
		
		// 'damage' might be between singular damages if exotic explosion resistances (e.g. negative ones) are used.
		// Limit the actual damage by the lower and upper clamp to be sure there's no damage multiplication done by collision groups. The most single dealt damage sets the bar.
		iobj->damage_hull(std::ranges::clamp(damageToRoot, minSingleRootDamage, maxSingleRootDamage), dmg);
	}

	bool __stdcall ExplosionHit(IObjRW* iobj, ExplosionDamageEvent* explosion, DamageList* dmg)
	{
		const float detonationDistance = FindDetonationDistance(explosion->projectileId) * 1.5f; // 1.5 multiplier to make sure the inner max-damage area covers some space beyond the detonation distance.
		if (DealShieldDamage(iobj, explosion, detonationDistance, dmg))
		{
			returncode = NOFUNCTIONCALL;
			return true;
		}

		if (explosion->explosionArchetype->fHullDamage == 0.0f)
		{
			returncode = NOFUNCTIONCALL;
			return true;
		}

		// Energy damage is being skipped because in FL:SR we don't do energy damage.
		DealEquipmentDamage(iobj, explosion, detonationDistance, dmg);
		DealHullAndColGrpDamage(iobj, explosion, detonationDistance, dmg);

		returncode = NOFUNCTIONCALL;
		return true;
	}
}