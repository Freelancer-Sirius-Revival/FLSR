#pragma once
#include <memory>

namespace Missions
{
	enum class CountComparator
	{
		Less,
		Equal,
		Greater
	};

	struct CndCountArchetype
	{
		unsigned int label = 0;
		unsigned int count = 0;
		CountComparator comparator = CountComparator::Equal;
	};
	typedef std::shared_ptr<CndCountArchetype> CndCountArchetypePtr;
}