#pragma once

namespace Missions
{
	struct ActAdjAcctArchetype
	{
		unsigned int objNameOrLabel = 0;
		unsigned int cash = 0;
	};
	typedef std::shared_ptr<ActAdjAcctArchetype> ActAdjAcctArchetypePtr;
}