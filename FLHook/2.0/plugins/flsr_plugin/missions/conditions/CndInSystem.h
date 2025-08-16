#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndInSystem : public Condition
	{
	private:
		const uint label;
		const uint systemId;

	public:
		CndInSystem(const ConditionParent& parent, const uint label, const uint systemId);
		~CndInSystem();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	namespace Hooks
	{
		namespace CndInSystem
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
			void __stdcall JumpInComplete_AFTER(unsigned int systemId, unsigned int shipId);
		}
	}
}