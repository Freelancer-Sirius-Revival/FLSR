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

		const auto& npcEntry = mission.npcs.find(msnNpcEntry->second.npcId);
		if (npcEntry == mission.npcs.end())
		{
			ConPrint(L"ERROR: NPC " + std::to_wstring(msnNpcId) + L" not found.\n");
			return objId;
		}

		const auto& msnNpc = msnNpcEntry->second;
		const auto& npc = npcEntry->second;

		ShipSpawning::NpcCreationParams params;
		params.archetypeId = npc.archetypeId;
		params.loadoutId = npc.loadoutId;
		params.position = positionOverride != nullptr ? *positionOverride : msnNpc.position;
		params.orientation = orientationOverride != nullptr ? *orientationOverride : msnNpc.orientation;
		params.systemId = msnNpc.systemId;
		params.hitpoints = msnNpc.hitpoints;
		params.level = npc.level;
		params.voiceId = msnNpc.voiceId;
		params.costume = msnNpc.costume;
		params.idsName = msnNpc.idsName;
		params.shipNameDisplayed = msnNpc.shipNameDisplayed;
		params.faction = npc.faction;
		params.stateGraphName = npc.stateGraph;
		params.pilotId = npc.pilotId;
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