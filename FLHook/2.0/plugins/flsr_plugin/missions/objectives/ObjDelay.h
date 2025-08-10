#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjDelay : public Objective
	{
	private:
		const float timeInS;

	public:
		ObjDelay(const ObjectiveParent& parent, const float timeInS);
		void Execute(const ObjectiveState& state) const;
	};
}