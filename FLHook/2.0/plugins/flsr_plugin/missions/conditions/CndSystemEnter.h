#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndSystemEnter : public Condition
	{
	private:
		const uint label;
		const uint systemId;

	public:
		CndSystemEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId);
		~CndSystemEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	namespace Hooks
	{
		namespace CndSystemEnter
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
		}
	}
}