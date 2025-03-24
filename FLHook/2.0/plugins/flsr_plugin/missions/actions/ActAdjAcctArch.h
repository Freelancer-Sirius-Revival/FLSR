#pragma once
#include <memory>

namespace Missions
{
	struct ActAdjAcctArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int cash = 0;
		bool splitBetweenPlayers = false;
	};
	typedef std::shared_ptr<ActAdjAcctArchetype> ActAdjAcctArchetypePtr;
}