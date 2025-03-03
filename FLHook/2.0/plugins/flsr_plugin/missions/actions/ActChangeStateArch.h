#pragma once

namespace Missions
{
	enum class MissionState
	{
		Fail,
		Succeed
	};

	struct ActChangeStateArchetype
	{
		MissionState state = MissionState::Fail;
		unsigned int failTextId = 0;
	};
	typedef std::shared_ptr<ActChangeStateArchetype> ActChangeStateArchetypePtr;
}