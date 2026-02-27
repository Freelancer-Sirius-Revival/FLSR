#pragma once
#include "Condition.h"

namespace Missions
{
	class CndSystemSpaceExit : public Condition
	{
	public:
		enum class SystemExitCondition
		{
			Any,
			Vanish,
			Explode,
			Jump,
			Dock
		};

	private:
		const uint label;
		const SystemExitCondition condition;
		const std::unordered_set<uint> systemIds;

	public:
		CndSystemSpaceExit(const ConditionParent& parent, const uint label, const SystemExitCondition condition, const std::unordered_set<uint>& systemIds);
		~CndSystemSpaceExit();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const SystemExitCondition reason);
	};

	namespace Hooks
	{
		namespace CndSystemSpaceExit
		{
			void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId);
			void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId);
			void __stdcall SystemSwitchOutComplete(unsigned int objId, unsigned int clientId);
		}
	}
}