#pragma once
#include <memory>
#include "ObjectiveState.h"

namespace Missions
{
	class Objective;
	typedef std::shared_ptr<Objective> ObjectivePtr;

	struct Objectives
	{
		const uint id;
		const uint missionId;
		std::vector<ObjectivePtr> objectives;

		Objectives(const uint id, const uint missionId);
		void Progress(const ObjectiveState& state) const;
	};
}