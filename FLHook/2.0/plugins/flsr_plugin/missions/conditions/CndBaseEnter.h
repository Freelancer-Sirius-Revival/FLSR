#pragma once
#include "Condition.h"

namespace Missions
{
	class CndBaseEnter : public Condition
	{
	private:
		const uint label;
		const uint baseId;

	public:
		CndBaseEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint baseId);
		~CndBaseEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint baseId);
	};

	namespace Hooks
	{
		namespace CndBaseEnter
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
		}
	}
}