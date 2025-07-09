#pragma once
#include "Action.h"

namespace Missions
{
	struct ActRandomPopSphere : Action
	{
		Vector position;
		float distance = 0.0f;
		uint systemId = 0;
		bool spawningAllowed = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActRandomPopSphere> ActRandomPopSpherePtr;

	void ClearRandomPopSpheres(const uint missionId);

	void __stdcall CShip_init(CShip* ship);
}