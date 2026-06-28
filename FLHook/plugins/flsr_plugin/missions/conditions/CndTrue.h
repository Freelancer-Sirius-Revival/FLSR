#pragma once
#include "Condition.h"

namespace Missions
{
	class CndTrue : public Condition
	{
	private:
		const uint activatorLabel;

	public:
		CndTrue(const ConditionParent& parent, const uint activatorLabel = 0);
		ConditionPtr Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const;
		void Register();
	};
}