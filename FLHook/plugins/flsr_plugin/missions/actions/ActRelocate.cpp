#include "ActRelocate.h"

namespace Missions
{
	static void RelocateNpc(const uint objId, const uint systemId, const Vector& position, Matrix orientation, const std::string& missionName)
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(objId, inspect, system) || inspect->cobj->objectClass != CObject::CSHIP_OBJECT)
		{
			ConPrint(L"ERROR: Msn " + stows(missionName) + L": Act_Relocate cannot move solar " + std::to_wstring(objId) + L"!\n");
			return;
		}

		if (orientation.data[0][0] == std::numeric_limits<float>::infinity())
		{
			Vector pos;
			pub::SpaceObj::GetLocation(objId, pos, orientation);
		}
		pub::SpaceObj::Relocate(objId, systemId, position, orientation);
	}

	void ActRelocate::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (objName == Activator)
		{
			if (activator.type == MissionObjectType::Client)
				HkRelocateClient(activator.id, position, orientation);
			else if (mission.objectIds.contains(activator.id))
				RelocateNpc(activator.id, systemId, position, orientation, mission.name);
		}
		else
		{
			if (const auto& objectByName = mission.objectIdsByName.find(objName); objectByName != mission.objectIdsByName.end())
				RelocateNpc(objectByName->second, systemId, position, orientation, mission.name);
		}
	}
}