#pragma once
#include "Objective.h"

namespace Missions
{
	class ObjSetLifeTime : public Objective
	{
	private:
		const float lifetime;

	public:
		ObjSetLifeTime(const ObjectiveParent& parent, const float lifetime);
		void Execute(const ObjectiveState& state) const;
	};
}