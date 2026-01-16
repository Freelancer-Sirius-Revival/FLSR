#pragma once
#include "Action.h"

namespace Missions
{
	struct ActSendComm : Action
	{
		uint id = 0;
		uint senderObjName = 0;
		uint receiverObjNameOrLabel = 0;
		std::vector<uint> lines;
		float delay = 0.5f;
		bool global = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActSendComm> ActSendCommPtr;
}