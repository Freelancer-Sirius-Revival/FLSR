#pragma once
#include "Condition.h"

namespace Missions
{
	class CndBaseEnter : public Condition
	{
	private:
		const uint label;
		const std::unordered_set<uint> baseIds;

	public:
		CndBaseEnter(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& baseIds);
		~CndBaseEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint currentBaseId);
	};

	namespace Hooks
	{
		namespace CndBaseEnter
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
		}
	}
}