#pragma once
#include <FLHook.h>

namespace Missions
{
	struct ActEtherCommArchetype
	{
		unsigned int name = 0;
		unsigned int senderVoiceId = 0;
		unsigned int senderIdsName = 0;
		unsigned int receiverObjNameOrLabel = 0;
		std::vector<unsigned int> lines;
		float delay = 0.5f;
		Costume costume;
		bool global = false;
	};
	typedef std::shared_ptr<ActEtherCommArchetype> ActEtherCommArchetypePtr;
}