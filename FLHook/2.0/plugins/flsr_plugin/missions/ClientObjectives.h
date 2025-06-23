#pragma once
#include <FLHook.h>
#include "../Plugin.h"

namespace Missions
{
	namespace ClientObjectives
	{
		struct Objective
		{
			uint missionId;
			Vector position;
			uint systemId;
			uint message;
			uint objId;
		};

		void DeleteClientObjectives(const uint clientId, const uint missionId);
		void SetClientObjective(const uint clientId, const Objective objective);
		void SendClientObjectives(const uint clientId);
		bool __stdcall Send_FLPACKET_COMMON_REQUEST_BEST_PATH(uint clientId, const XRequestBestPath& data, int size);
		void __stdcall Elapse_Time_AFTER(float seconds);
	}
}