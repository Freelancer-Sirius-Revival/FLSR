#include "ActDockInstant.h"

namespace Missions
{
	static void DockInstant(const uint clientId, const uint targetIdOrName, const std::string& dockHardpoint, const Mission& mission)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;

		uint shipId;
		pub::Player::GetShip(clientId, shipId);
		if (!shipId)
			return;

		uint targetObjId = 0;
		int reputationId;
		// Try to find any solar in the entire game first.
		pub::SpaceObj::GetSolarRep(targetIdOrName, reputationId);
		// Solar IDs are the exact same as their reputation ID
		if (reputationId != 0 && targetIdOrName == reputationId)
			targetObjId = targetIdOrName;
		else if (const auto& objectByName = mission.objectIdsByName.find(targetIdOrName); objectByName != mission.objectIdsByName.end())
			targetObjId = objectByName->second;

		if (targetObjId == 0)
			return;

		if (pub::SpaceObj::ExistsAndAlive(targetObjId) != 0)
			return;

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(targetObjId, inspect, system) || inspect->cobj->objectClass != CObject::CSOLAR_OBJECT)
			return;

		const auto archetype = static_cast<Archetype::EqObj*>(inspect->cobj->archetype);
		for (int index = 0, length = archetype->dockInfo.size(); index < length; index++)
		{
			if (std::string(archetype->dockInfo[index].hardpoint) == dockHardpoint)
			{
				pub::SpaceObj::InstantDock(shipId, targetObjId, index);
				return;
			}
		}
	}

	void ActDockInstant::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (label == Activator)
		{
			if (activator.type == MissionObjectType::Client && activator.id)
				DockInstant(activator.id, targetObjName, dockHardpoint, mission);
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
					DockInstant(object.id, targetObjName, dockHardpoint, mission);
			}
		}
	}
}