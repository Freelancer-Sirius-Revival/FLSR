#pragma once
#include <FLHook.h>

namespace ShipSpawning
{
	struct NpcCreationParams
	{
		uint archetypeId;
		uint loadoutId;
		Vector position;
		Matrix orientation;
		uint systemId;
		int hitpoints = -1;
		uint level;
		uint voiceId = 0;
		Costume costume;
		uint formationIdsName = 0;
		uint idsName = 0;
		bool shipNameDisplayed = false;
		std::string faction = "";
		std::string stateGraphName;
		uint pilotId;
		uint pilotJobId = 0;
		uint launchObjId = 0;
	};

	void AssignToWing(const uint shipId, const uint wingLeaderShipId);
	void UnassignFromWing(const uint shipId);
	void SetLifeTime(const uint shipId, const float lifeTime);
	float GetLifeTime(const uint shipId);
	uint CreateNPC(const NpcCreationParams& params);

	void __stdcall PlayerLaunch_AFTER(unsigned int shipId, unsigned int clientId);
	void __stdcall SystemSwitchOutComplete_AFTER(unsigned int shipId, unsigned int clientId);
}