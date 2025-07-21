#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjDelay : public Objective
	{
	private:
		const uint timeInS;

	public:
		ObjDelay(const ObjectiveParent& parent, const int objectiveIndex, const uint timeInS);
		void Execute(const uint objId) const;
	};
}