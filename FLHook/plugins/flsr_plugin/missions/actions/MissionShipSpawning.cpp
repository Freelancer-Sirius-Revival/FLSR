#include "MissionShipSpawning.h"
#include "../ShipSpawning.h"
#include "../../NpcCloaking.h"

namespace Missions
{
	uint SpawnShip(const uint msnNpcId, Mission& mission, const Vector* positionOverride, const Matrix* orientationOverride)
	{
		uint objId = 0;
		if (mission.objectIdsByName.contains(msnNpcId))
			return objId;

		const auto& msnNpcEntry = mission.msnNpcs.find(msnNpcId);
		if (msnNpcEntry == mission.msnNpcs.end())
		{
			ConPrint(L"ERROR: MSN NPC " + std::to_wstring(msnNpcId) + L" not found.\n");
			return objId;
		}

		NpcShipArchetypes::NpcShipArch npcShipArch;
		if (const auto& npcEntry = mission.npcShipArchetypes.find(msnNpcEntry->second.npcShipArchId); npcEntry != mission.npcShipArchetypes.end())
			npcShipArch = npcEntry->second;
		else if(!NpcShipArchetypes::GetNpcShipArch(msnNpcEntry->second.npcShipArchId, npcShipArch))
		{
			ConPrint(L"ERROR: NpcShipArch " + std::to_wstring(msnNpcId) + L" not found.\n");
			return objId;
		}

		const auto& msnNpc = msnNpcEntry->second;

		ShipSpawning::NpcCreationParams params;
		params.archetypeId = npcShipArch.archetypeId;
		params.loadoutId = npcShipArch.loadoutId;
		params.position = positionOverride != nullptr ? *positionOverride : msnNpc.position;
		params.orientation = orientationOverride != nullptr ? *orientationOverride : msnNpc.orientation;
		params.systemId = msnNpc.systemId;
		params.hitpoints = msnNpc.hitpoints;
		params.level = npcShipArch.level;
		params.voiceId = msnNpc.voiceId;
		params.costume = msnNpc.costume;
		params.idsName = msnNpc.idsName;
		params.shipNameDisplayed = msnNpc.shipNameDisplayed;
		params.faction = msnNpc.faction;
		params.stateGraphName = npcShipArch.stateGraph;
		params.pilotId = npcShipArch.pilotId;
		params.pilotJobId = msnNpc.pilotJobId;
		if (positionOverride == nullptr && orientationOverride == nullptr)
		{
			const auto& foundObjectEntry = mission.objectIdsByName.find(msnNpc.startingObjId);
			params.launchObjId = foundObjectEntry != mission.objectIdsByName.end() ? foundObjectEntry->second : msnNpc.startingObjId;
		}

		objId = ShipSpawning::CreateNPC(params);

		if (objId)
		{
			mission.AddObject(objId, msnNpcId, msnNpcEntry->second.labels);
			NpcCloaking::RegisterObject(objId);
		}
		else
			ConPrint(L"ERROR: MSN NPC " + std::to_wstring(msnNpc.id) + L" in system " + std::to_wstring(params.systemId) + L" at position " + std::to_wstring(params.position.x) + L", " + std::to_wstring(params.position.y) + L", " + std::to_wstring(params.position.z) + L"\n");

		return objId;
	}
}