#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndInSpace : public Condition
	{
	private:
		const uint label;
		const std::unordered_set<uint> systemIds;

	public:
		CndInSpace(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& systemIds);
		~CndInSpace();
		void Register();
		void Unregister();
		bool Matches(const uint clientId);
	};

	namespace Hooks
	{
		namespace CndInSpace
		{
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
			void __stdcall SystemSwitchOutComplete_AFTER(unsigned int objId, unsigned int clientId);
		}
	}
}