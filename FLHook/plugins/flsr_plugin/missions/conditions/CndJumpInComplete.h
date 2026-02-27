#pragma once
#include "Condition.h"

namespace Missions
{
	class CndJumpInComplete : public Condition
	{
	private:
		const uint label;
		const std::unordered_set<uint> systemIds;

	public:
		CndJumpInComplete(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& systemIds);
		~CndJumpInComplete();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint currentSystemId);
	};

	namespace Hooks
	{
		namespace CndJumpInComplete
		{
			void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId);
		}
	}
}