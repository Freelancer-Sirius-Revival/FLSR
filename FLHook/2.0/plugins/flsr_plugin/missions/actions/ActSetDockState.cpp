#include "ActSetDockState.h"

namespace Missions
{
	static void SetDockState(uint objId, const std::string& dockHardpoint, const bool active)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system) || inspect->cobj->objectClass != CObject::CSOLAR_OBJECT)
			return;

		const auto archetype = static_cast<Archetype::EqObj*>(inspect->cobj->archetype);
		for (int index = 0, length = archetype->dockInfo.size(); index < length; index++)
		{
			if (std::string(archetype->dockInfo[index].hardpoint) == dockHardpoint)
			{
				pub::SpaceObj::Activate(objId, active, index);
				return;
			}
		}
	}

	void ActSetDockState::Execute(Mission& mission, const MissionObject& activator) const
	{
		int reputationId;
		if (objNameOrLabel == Activator)
		{
			if (activator.type == MissionObjectType::Object)
			{
				pub::SpaceObj::GetSolarRep(activator.id, reputationId);
				if (reputationId != 0 && objNameOrLabel == reputationId)
					SetDockState(activator.id, dockHardpoint, opened);
			}
		}
		else
		{
			// Try to find any solar in the entire game first.
			pub::SpaceObj::GetSolarRep(objNameOrLabel, reputationId);
			// Solar IDs are the exact same as their reputation ID
			if (reputationId != 0 && objNameOrLabel == reputationId)
				SetDockState(objNameOrLabel, dockHardpoint, opened);
			// Clients can only be addressed via Label.
			else if (const auto& objectByName = mission.objectIdsByName.find(objNameOrLabel); objectByName != mission.objectIdsByName.end())
			{
				SetDockState(objectByName->second, dockHardpoint, opened);
			}
			else if (const auto& objectsByLabel = mission.objectsByLabel.find(objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
					SetDockState(object.id, dockHardpoint, opened);
			}
		}
	}
}