#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndInSystem : public Condition
	{
	private:
		const uint label;
		const std::unordered_set<uint> systemIds;

	public:
		CndInSystem(const ConditionParent& parent, const uint label, const std::unordered_set<uint>& systemIds);
		~CndInSystem();
		void Register();
		void Unregister();
		bool Matches(const uint clientId);
	};

	namespace Hooks
	{
		namespace CndInSystem
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
			void __stdcall SystemSwitchOutComplete_AFTER(unsigned int objId, unsigned int clientId);
		}
	}
}