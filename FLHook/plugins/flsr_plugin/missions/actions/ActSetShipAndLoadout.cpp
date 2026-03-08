#include "ActSetShipAndLoadout.h"

namespace Missions
{
	static void SetShipAndLoadout(const uint clientId, const uint shipArchetypeId, const uint loadoutId)
	{
		uint baseId = 0;
		pub::Player::GetBase(clientId, baseId);

		if (!baseId)
			return;

		EquipDescVector equipVector;
		const auto& loadout = Loadout::Get(loadoutId);
		auto entry = loadout->first;
		while (entry != loadout->end)
		{
			equipVector.equip.push_back(*entry);
			entry++;
		}
		pub::Player::SetShipAndLoadout(clientId, shipArchetypeId, equipVector);
	}

	void ActSetShipAndLoadout::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				SetShipAndLoadout(activator.id, shipArchetypeId, loadoutId);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					SetShipAndLoadout(object.id, shipArchetypeId, loadoutId);
			}
		}
	}
}