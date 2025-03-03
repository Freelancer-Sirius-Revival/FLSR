#pragma once

namespace Missions
{
	const enum DestroyedCondition
	{
		ALL,
		SILENT,
		EXPLODE
	};

	struct CndDestroyedArchetype
	{
		unsigned int objNameOrLabel = 0;
		int count = 0;
		DestroyedCondition condition = DestroyedCondition::ALL;
		unsigned int killerNameOrLabel = 0;
	};
	typedef std::shared_ptr<CndDestroyedArchetype> CndDestroyedArchetypePtr;
}