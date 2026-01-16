#pragma once
#include "Action.h"

namespace Missions
{
	struct ActEtherComm : Action
	{
		uint id = 0;
		uint senderVoiceId = 0;
		uint senderIdsName = 0;
		uint receiverObjNameOrLabel = 0;
		std::vector<uint> lines;
		float delay = 0.5f;
		Costume costume;
		bool global = false;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActEtherComm> ActEtherCommPtr;
}