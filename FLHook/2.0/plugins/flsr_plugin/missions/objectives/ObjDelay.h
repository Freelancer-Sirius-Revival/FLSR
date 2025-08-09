#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjDelay : public Objective
	{
	private:
		const float timeInS;

	public:
		ObjDelay(const ObjectiveParent& parent, const int objectiveIndex, const float timeInS);
		void Execute(const uint objId) const;
	};
}