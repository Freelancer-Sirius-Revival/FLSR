#pragma once
#include <FLHook.h>

struct NpcCreationParams
{
	uint archetypeId;
	uint loadoutId;
	Vector position;
	Matrix orientation;
	uint systemId;
	int hitpoints;
	uint level;
	uint voiceId = 0;
	Costume costume;
	uint idsName = 0;
	std::string faction = "";
	std::string stateGraphName;
	uint pilotId;
	uint pilotJobId = 0;
	uint launchObjId = 0;
};

void AssignToWing(const uint shipId, const uint wingLeaderShipId);
void UnassignFromWing(const uint shipId);
uint CreateNPC(const NpcCreationParams& params);