#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjMakeNewFormation : public Objective
	{
	private:
		const uint formationId;
		const std::vector<uint> objNames;

	public:
		ObjMakeNewFormation(const ObjectiveParent& parent, const uint formationId, const std::vector<uint>& objNames);
		void Execute(const ObjectiveState& state) const;
	};
}