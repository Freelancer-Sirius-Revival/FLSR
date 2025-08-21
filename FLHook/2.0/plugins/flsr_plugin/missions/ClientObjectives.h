#pragma once
#include <FLHook.h>
#include "../Plugin.h"

namespace Missions
{
	namespace ClientObjectives
	{
		struct Objective
		{
			uint missionId = 0;
			Vector position;
			uint systemId = 0;
			uint message = 0;
			uint objId = 0;
			bool bestPath = false;
		};

		void DeleteClientObjectives(const uint clientId, const uint missionId);
		bool DoesClientHaveObjective(const uint clientId);
		void SetClientObjective(const uint clientId, const Objective objective);
		void SendClientObjectives(const uint clientId);
		bool __stdcall Send_FLPACKET_COMMON_REQUEST_BEST_PATH(uint clientId, const XRequestBestPath& data, int size);
		void __stdcall Elapse_Time_AFTER(float seconds);
	}
}