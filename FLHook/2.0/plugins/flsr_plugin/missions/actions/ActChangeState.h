#pragma once
#include "Action.h"

namespace Missions
{
	enum class ChangeState
	{
		Succeed,
		Fail
	};

	struct ActChangeState : Action
	{
		ChangeState state = ChangeState::Fail;
		uint failureStringId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActChangeState> ActChangeStatePtr;
}