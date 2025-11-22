#include "PlayerLootSpawning.h"
#include "Plugin.h"

namespace PlayerLootSpawning
{
	void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		if (!killed || !killedObject->is_player() || !(killedObject->cobj->objectClass & CObject::CSHIP_OBJECT))
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		CShip* ship = reinterpret_cast<CShip*>(killedObject->cobj);
		const int maxSpawnRadius = std::max<int>(2, std::ceil(ship->radiusCentered / 4.0f));

		pub::SpaceObj::LootInfo loot;
		loot.systemId = ship->system;
		loot.ownerId = ship->id;
		loot.infocardOverride = 0;
		loot.initialAngular = ship->get_angular_velocity();
		loot.hitPtsPercentage = 1.0f;
		loot.canAITractor = true;
		loot.isMissionLoot = false;

		auto& equipDescList = Players[ship->ownerPlayer].equipDescList;
		for (const auto& equip : equipDescList.equip)
		{
			if (equip.bMounted)
				continue;

			const auto& archetype = Archetype::GetEquipment(equip.iArchID);
			if (!equip.bMission && archetype->bLootable)
			{
				const auto unitsPerContainer = archetype->iUnitsPerContainer;
				loot.equipmentArchId = equip.iArchID;
				uint remainingCargo = equip.iCount;
				uint lootCount = 0; // Hard cut to prevent any sort of heavy performance when spawning too much loot.
				while (remainingCargo > 0 && lootCount < 20)
				{
					uint lootId = 0;
					const auto& randomAngles = RandomVector(180.0f);
					loot.rot = EulerMatrix(randomAngles);
					const float randomSpawnFactor = std::max<int>(1, std::rand() % maxSpawnRadius);
					const Vector randomPos = RandomVector(randomSpawnFactor);
					loot.pos.x = ship->vPos.x + randomPos.x;
					loot.pos.y = ship->vPos.y + randomPos.y;
					loot.pos.z = ship->vPos.z + randomPos.z;
					const Vector randomVelocity = RandomVector(std::min<float>(randomSpawnFactor * 4.0f, 100.0f));
					loot.initialVelocity = ship->get_velocity();
					loot.initialVelocity.x += randomVelocity.x;
					loot.initialVelocity.y += randomVelocity.y;
					loot.initialVelocity.z += randomVelocity.z;
					loot.itemCount = std::min<uint>(1000, std::min<uint>(remainingCargo, unitsPerContainer));
					remainingCargo -= loot.itemCount;
					pub::SpaceObj::CreateLoot(lootId, loot);
					lootCount++;
				}
			}

			equipDescList.remove_equipment_item(equip.sID, equip.iCount);
		}
		ship->clear_equip_and_cargo();
		returncode = DEFAULT_RETURNCODE;
	}
}