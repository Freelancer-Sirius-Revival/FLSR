#pragma once
#include <memory>

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
		void Progress(const uint objId, const int index) const;
	};
}