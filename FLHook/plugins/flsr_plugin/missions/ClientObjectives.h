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

		void ClearClientObjectives(const uint clientId, const uint missionId);
		bool DoesClientHaveObjective(const uint clientId);
		void SetClientObjective(const uint clientId, const Objective objective);
		bool __stdcall Send_FLPACKET_COMMON_REQUEST_BEST_PATH(uint clientId, const XRequestBestPath& data, int size);
		void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
		void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
		void __stdcall GoTradelane_AFTER(unsigned int clientId, const XGoTradelane& tradeLaneInfo);
		void __stdcall StopTradelane_AFTER(unsigned int clientId, unsigned int p2, unsigned int p3, unsigned int p4);
		void __stdcall Elapse_Time_AFTER(float seconds);
	}
}