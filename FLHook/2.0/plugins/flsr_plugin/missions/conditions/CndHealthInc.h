#pragma once
#include <FLHook.h>
#include "Condition.h"

namespace Missions
{
	class CndHealthInc : public Condition
	{
	private:
		const uint objNameOrLabel;
		const float relativeHitpointsThreshold;
		const std::unordered_set<uint> colGrpIds;
		const bool repairedIsActivator;

	public:
		CndHealthInc(const ConditionParent& parent,
						const uint objNameOrLabel,
						const float relativeHitpointsThreshold,
						const std::unordered_set<uint> colGrpIds,
						const bool repairedIsActivator);
		~CndHealthInc();
		void Register();
		void Unregister();
		bool Matches(const IObjRW* damagedObject, const float incomingDamage, const DamageList* damageList, const CArchGroup* hitColGrp);
	};

	namespace Hooks
	{
		namespace CndHealthInc
		{
			void __stdcall ShipAndSolarColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList);
			void __stdcall ShipAndSolarHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList);
		}
	}
}