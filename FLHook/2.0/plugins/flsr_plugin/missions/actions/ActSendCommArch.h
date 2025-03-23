#pragma once
#include <memory>
#include <vector>

namespace Missions
{
	struct ActSendCommArchetype
	{
		unsigned int name = 0;
		unsigned int senderObjName = 0;
		unsigned int receiverObjNameOrLabel = 0;
		std::vector<unsigned int> lines;
		float delay = 0.5f;
		bool global = false;
	};
	typedef std::shared_ptr<ActSendCommArchetype> ActSendCommArchetypePtr;
}