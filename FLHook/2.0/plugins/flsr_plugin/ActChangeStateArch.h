#pragma once

namespace Missions
{
	const enum MissionState
	{
		FAIL,
		SUCCEED,
		ABORT
	};

	struct ActChangeStateArchetype
	{
		MissionState state = MissionState::FAIL;
		unsigned int failTextId = 0;
	};
}