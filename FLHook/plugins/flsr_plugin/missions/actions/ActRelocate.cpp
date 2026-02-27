#include "ActRelocate.h"

namespace Missions
{
	static void RelocateNpc(const uint objId, const Vector& position, Matrix orientation)
	{
		uint systemId;
		pub::SpaceObj::GetSystem(objId, systemId);
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
				RelocateNpc(activator.id, position, orientation);
		}
		else
		{
			if (const auto& objectByName = mission.objectIdsByName.find(objName); objectByName != mission.objectIdsByName.end())
				RelocateNpc(objectByName->second, position, orientation);
		}
	}
}