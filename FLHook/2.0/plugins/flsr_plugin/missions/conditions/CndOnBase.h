#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndOnBase : public Condition
	{
	private:
		const uint label;
		const uint baseId;

	public:
		CndOnBase(const ConditionParent& parent, const uint label, const uint baseId);
		~CndOnBase();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint currentBaseId);
	};

	namespace Hooks
	{
		namespace CndOnBase
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
		}
	}
}