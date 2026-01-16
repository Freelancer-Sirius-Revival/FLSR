#pragma once
#include "Condition.h"

namespace Missions
{
	struct CndSystemSpaceEnter : public Condition
	{
	public:
		enum class SystemEnterCondition
		{
			Any,
			Jump,
			Launch,
			Spawn
		};

	private:
		const uint label;
		const SystemEnterCondition condition;
		const std::unordered_set<uint> systemIds;

	public:
		CndSystemSpaceEnter(const ConditionParent& parent, const uint label, const SystemEnterCondition condition, const std::unordered_set<uint>& systemIds);
		~CndSystemSpaceEnter();
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const SystemEnterCondition reason);
	};

	namespace Hooks
	{
		namespace CndSystemSpaceEnter
		{
			void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
			void __stdcall CharacterInfoReq(unsigned int clientId, bool p2);
			void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2);
			void __stdcall PlayerLaunch_AFTER(unsigned int objId, unsigned int clientId);
			void __stdcall SystemSwitchOutComplete_AFTER(unsigned int objId, unsigned int clientId);
		}
	}
}