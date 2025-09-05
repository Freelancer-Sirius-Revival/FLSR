#pragma once
#include "Condition.h"

namespace Missions
{
	class CndBaseExit : public Condition
	{
	private:
		const uint label;
		const std::unordered_set<uint> baseIds;

	public:
		CndBaseExit(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& baseIds);
		~CndBaseExit();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint currentBaseId);
	};

	namespace Hooks
	{
		namespace CndBaseExit
		{
			void __stdcall BaseExit_AFTER(unsigned int baseId, unsigned int clientId);
		}
	}
}