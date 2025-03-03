#pragma once

namespace Missions
{
	const enum MissionState
	{
		FAIL,
		SUCCEED
	};

	struct ActChangeStateArchetype
	{
		MissionState state = MissionState::FAIL;
		unsigned int failTextId = 0;
	};
	typedef std::shared_ptr<ActChangeStateArchetype> ActChangeStateArchetypePtr;
}