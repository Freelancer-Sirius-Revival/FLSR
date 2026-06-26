#include "CargoPods.h"
#include "Plugin.h"
#include <random>
#include <algorithm>

namespace CargoPods
{
	std::mt19937 randomizer(std::random_device{}());

	const uint defaultCargoPod = CreateID("cargopod_grey");
	const uint lostCargoPodId = CreateID("lost_cargopod");

	std::unordered_map<uint, std::unordered_set<std::string>> cargoPodHpNamesByShipArchId;

	static bool LazyRegisterCargoPodHardpoints(const uint shipArchId)
	{
		if (cargoPodHpNamesByShipArchId.contains(shipArchId))
			return true;

		const auto shipArch = Archetype::GetShip(shipArchId);
		if (!shipArch)
			return false;

		auto& hpNames = cargoPodHpNamesByShipArchId[shipArchId]; // Implicitely also registers the shiparch ID as known.
		for (const auto& hardpoint : shipArch->hardpoints)
		{
			if (hardpoint.type == HpAttachmentType::hp_cargo_pod)
			{
				for (const auto& hpName : hardpoint.hp)
					hpNames.insert(hpName.value);
			}
		}
		return true;
	}

	void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId)
	{
		uint shipArchId;
		pub::Player::GetShipID(clientId, shipArchId);
		if (!shipArchId || !LazyRegisterCargoPodHardpoints(shipArchId))
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		const auto& hpNames = cargoPodHpNamesByShipArchId.at(shipArchId);
		if (hpNames.empty())
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		const auto& equipDescList = Players[clientId].equipDescList;
		for (const auto& equip : equipDescList.equip)
		{
			if (equip.iArchID == lostCargoPodId)
			{
				pub::Player::RemoveCargo(clientId, equip.sID, 1);
			}
			else if (hpNames.contains(equip.szHardPoint.value))
			{
				const auto& equipment = Archetype::GetEquipment(equip.iArchID);
				if (equipment && equipment->get_class_type() == Archetype::AClassType::CARGO_POD)
					pub::Player::RemoveCargo(clientId, equip.sID, 1);
			}
		}

		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		uint shipArchId;
		pub::Player::GetShipID(clientId, shipArchId);
		if (!shipArchId || !LazyRegisterCargoPodHardpoints(shipArchId))
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		const auto& hpNames = cargoPodHpNamesByShipArchId.at(shipArchId);
		if (hpNames.empty())
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		std::unordered_map<uint, uint> countByPodId;
		const auto& equipDescList = Players[clientId].equipDescList;
		for (const auto& equip : equipDescList.equip)
		{
			if (!equip.bMounted && std::string(equip.szHardPoint.value) == std::string(EquipDesc::CARGO_BAY_HP_NAME.value))
			{
				const auto& equipment = Archetype::GetEquipment(equip.iArchID);
				if (equipment && equipment->get_class_type() == Archetype::AClassType::COMMODITY)
				{
					const auto& archetype = static_cast<Archetype::Commodity*>(equipment);
					if (!archetype->podAppearance)
						continue;
					if (auto entry = countByPodId.find(archetype->podAppearance->iArchID); entry != countByPodId.end())
						entry->second++;
					else
						countByPodId.insert({ archetype->podAppearance->iArchID, 1 });
				}
			}
		}

		std::pair<uint, uint> mostUsedCargoPodId({ defaultCargoPod, 0 });
		for (const auto& entry : countByPodId)
		{
			if (entry.second > mostUsedCargoPodId.second)
				mostUsedCargoPodId = entry;
		}

		// At this point there MUST always be a cargo pod, even if just a fallback if no relevant cargo is present.
		for (const auto& hpName : hpNames)
			HkAddEquip(ARG_CLIENTID(clientId), mostUsedCargoPodId.first, hpName);

		returncode = DEFAULT_RETURNCODE;
	}

	struct LootItem
	{
		ushort id;
		uint archetypeId;
		uint count;
		float unitVolume;
		float totalVolume;
		bool lootable;
		uint unitsPerContainer;
	};

	const uint NNComponentDestroyedId = CreateID("component_destroyed");
	const uint NNWarningId = CreateID("warning");
	const uint NNCargoJettisoned = CreateID("cargo_jettisoned");

	void __stdcall ShipEquipDestroyed(const IObjRW* iobj, const CEquip* equip, const DamageEntry::SubObjFate fate, const DamageList* dmgList)
	{
		const uint clientId = iobj->cobj->ownerPlayer;
		if (!clientId || equip->CEquipType != EquipmentClass::CargoPod)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		const CShip* ship = reinterpret_cast<CShip*>(iobj->cobj);
		if (ship->archetype && cargoPodHpNamesByShipArchId.contains(ship->archetype->iArchID))
		{
			// Collect all relevant items.
			std::vector<LootItem> lootableItems;
			float occupiedCargoHold = 0.0f;
			uint existingCargoPodCount = 0;
			auto& equipDescList = Players[clientId].equipDescList;
			for (const auto& equip : equipDescList.equip)
			{
				if (equip.bMounted && cargoPodHpNamesByShipArchId.at(ship->archetype->iArchID).contains(equip.szHardPoint.value))
				{
					const auto& equipment = Archetype::GetEquipment(equip.iArchID);
					if (equipment && equipment->get_class_type() == Archetype::AClassType::CARGO_POD)
						existingCargoPodCount++;
				}

				if (!equip.bMounted && !equip.bMission && equip.iArchID != lostCargoPodId)
				{
					const auto& archetype = Archetype::GetEquipment(equip.iArchID);
					if (archetype && archetype->fVolume > 0.0f)
					{
						const float totalVolume = equip.iCount * archetype->fVolume;
						lootableItems.emplace_back(equip.sID, equip.iArchID, equip.iCount, archetype->fVolume, totalVolume, archetype->bLootable, archetype->iUnitsPerContainer);
						occupiedCargoHold += totalVolume;
					}
				}
			}

			// Inform the player about the cargo pod and potential contents lost.
			if (!lootableItems.empty())
			{
				pub::Player::SendNNMessage(clientId, NNWarningId);
				pub::Player::SendNNMessage(clientId, NNCargoJettisoned);
			}
			else
				pub::Player::SendNNMessage(clientId, NNComponentDestroyedId);

			// If there is more than one cargo pod, calculate the relative loot count. Otherwise, the entire rest of the items must be dropped anyway.
			if (existingCargoPodCount > 1)
			{

				// Sort items primarily by their count and secondly by their individual volume. Biggest first.
				std::sort(lootableItems.begin(), lootableItems.end(), [](LootItem a, LootItem b) { return a.totalVolume > b.totalVolume; });

				// Target volume is a portion of what we have in the ship right now.
				const float targetVolumeToLoot = occupiedCargoHold / existingCargoPodCount;
				float currentVolumeToLoot = 0.0f;

				// Every found stack will have an equal percentage of items to loot.
				for (auto& item : lootableItems)
				{
					if (currentVolumeToLoot < targetVolumeToLoot)
					{
						// Always ceil the resulting count to be sure there is always at least enough volume to drop.
						item.count = std::min<uint>(item.count, std::ceil(((item.totalVolume / occupiedCargoHold) * targetVolumeToLoot) / item.unitVolume));
						currentVolumeToLoot += item.count * item.unitVolume;
					}
					else
					{
						// The target volume was reacheD due to rounding up the volume in the previous steps.
						// Mark this item with count 0 and end the loop. There's no need to find more items to drop.
						item.count = 0;
						break;
					}
				}
			}

			const auto& cargoPod = static_cast<const CECargoPod*>(equip);
			Vector cargoPodPosition;
			cargoPod->GetCenterOfMass(cargoPodPosition);
			Vector connectionPosition;
			Matrix connectionOrientation;
			if (!cargoPod->GetConnectionPosition(&connectionPosition, &connectionOrientation))
			{
				connectionPosition = ship->get_position();
				connectionOrientation = ship->get_orientation();
			}
			const Vector shipVelocity = ship->get_velocity();
			const float fromShipAwaySpeedFactor = 5.0f;
			const Vector velocity({
				shipVelocity.x + (cargoPodPosition.x - connectionPosition.x) * fromShipAwaySpeedFactor,
				shipVelocity.y + (cargoPodPosition.y - connectionPosition.y) * fromShipAwaySpeedFactor,
				shipVelocity.z + (cargoPodPosition.z - connectionPosition.z) * fromShipAwaySpeedFactor
			});
			float radius;
			cargoPod->GetRadius(radius);
			const int maxSpawnRadius = std::max<int>(2, std::ceil(radius / 4.0f));

			pub::SpaceObj::LootInfo loot;
			loot.systemId = ship->system;
			loot.ownerId = ship->id;
			loot.infocardOverride = 0;
			loot.initialAngular = ship->get_angular_velocity();
			loot.hitPtsPercentage = 1.0f;
			loot.canAITractor = true;
			loot.isMissionLoot = false;

			for (const auto& item : lootableItems)
			{
				// Items with zero count mark the end of the sorted list of items that should be dropped.
				if (item.count == 0)
					break;

				pub::Player::RemoveCargo(clientId, item.id, item.count);

				if (!item.lootable)
					continue;

				const auto unitsPerContainer = item.unitsPerContainer;
				loot.equipmentArchId = item.archetypeId;
				uint remainingCargo = item.count;
				uint lootCount = 0; // Hard cut to prevent any sort of heavy performance when spawning too much loot.
				while (remainingCargo > 0 && lootCount < 20)
				{
					uint lootId = 0;
					const auto& randomAngles = RandomVector(180.0f);
					loot.rot = EulerMatrix(randomAngles);
					const float randomSpawnFactor = std::max<int>(1, std::rand() % maxSpawnRadius);
					const Vector randomPos = RandomVector(randomSpawnFactor);
					loot.pos.x = cargoPodPosition.x + randomPos.x;
					loot.pos.y = cargoPodPosition.y + randomPos.y;
					loot.pos.z = cargoPodPosition.z + randomPos.z;
					const Vector randomVelocity = RandomVector(std::min<float>(randomSpawnFactor * 4.0f, 100.0f));
					loot.initialVelocity = velocity;
					loot.initialVelocity.x += randomVelocity.x;
					loot.initialVelocity.y += randomVelocity.y;
					loot.initialVelocity.z += randomVelocity.z;
					loot.itemCount = std::min<uint>(1000, std::min<uint>(remainingCargo, unitsPerContainer));
					remainingCargo -= loot.itemCount;
					pub::SpaceObj::CreateLoot(lootId, loot);
					lootCount++;
				}
			}

			// At last, add the blocking cargo item.
			pub::Player::AddCargo(clientId, lostCargoPodId, 1, 1.0f, false);
		}

		returncode = DEFAULT_RETURNCODE;
	}
}