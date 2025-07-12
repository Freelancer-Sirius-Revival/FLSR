#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndSpaceEnter : public Condition
	{
	private:
		const uint label;
		const uint systemId;

	public:
		CndSpaceEnter(const ConditionParent& parent, const uint objNameOrLabel, const uint systemId);
		~CndSpaceEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	namespace Hooks
	{
		namespace CndSpaceEnter
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
		}
	}
}