#pragma once
#include "Condition.h"

namespace Missions
{
	class CndSystemExit : public Condition
	{
	private:
		const uint label;
		const uint systemId;

	public:
		CndSystemExit(const ConditionParent& parent, const uint label, const uint systemId);
		~CndSystemExit();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint systemId);
	};

	namespace Hooks
	{
		namespace CndSystemExit
		{
			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
		}
	}
}