#include "Equipment.h"
#include "../Plugin.h"

namespace AntiCheat
{
	static bool IsHardpointValid(const Archetype::Ship* shipArch, const char* targetHpName, const HpAttachmentType targetHpType)
	{
		const size_t hpNameLength = std::strlen(targetHpName);
		for (const auto& hardpoint : shipArch->hardpoints)
		{
			if (hardpoint.type != targetHpType)
				continue;
			for (const auto& possibleHardpointNames : hardpoint.hp)
			{
				if (std::strlen(possibleHardpointNames.value) == hpNameLength && std::strncmp(possibleHardpointNames.value, targetHpName, hpNameLength) == 0)
					return true;
			}
		}
		return false;
	}

	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
	{
		const uint shipArchId = Players[clientId].iShipArchetype;
		if (!shipArchId)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		const Archetype::Ship* shipArch = Archetype::GetShip(shipArchId);
		if (!shipArch)
		{
			returncode = DEFAULT_RETURNCODE;
			return;
		}

		std::unordered_set<char*> occupiedHardpoints;
		const auto& equipList = Players[clientId].equipDescList;
		for (const auto& equip : equipList.equip)
		{
			if (equip.bMission)
				continue;

			const Archetype::Equipment* equipArch = Archetype::GetEquipment(equip.iArchID);
			const HpAttachmentType hpType = equipArch->get_hp_type();

			HpAttachmentType hpSubType = HpAttachmentType::EXTERNAL;
			switch (equipArch->get_class_type())
			{
				case Archetype::GUN:
					hpSubType = reinterpret_cast<const Archetype::Gun*>(equipArch)->get_hp_type_by_index(0);
					break;
				case Archetype::SHIELD_GENERATOR:
					hpSubType = reinterpret_cast<const Archetype::ShieldGenerator*>(equipArch)->get_hp_type_by_index(0);
					break;
			}

			if (equip.bMounted)
			{
				//const Archetype::Equipment* equipArch = Archetype::GetEquipment(equip.iArchID);
				if (!equipArch)
				{
					returncode = DEFAULT_RETURNCODE;
					return;
				}
				//const HpAttachmentType hpType = equipArch->get_hp_type();
				if (hpType && IsHardpointValid(shipArch, equip.szHardPoint.value, hpType))
				{
					occupiedHardpoints.insert(equip.szHardPoint.value);
				}
			}
		}

		returncode = DEFAULT_RETURNCODE;
		return;
	}
}