#pragma once
#include "Condition.h"

namespace Missions
{
	class CndProjHitCount : public Condition
	{
	public:
		enum class DamagedSurface
		{
			Any,
			Hull,
			Shield
		};

		enum class DamageType
		{
			Any,
			Projectile,
			Explosion
		};

	private:
		const uint damagedObjNameOrLabel;
		const DamagedSurface targetSurface;
		const DamageType damageType;
		const uint targetHitCount;
		const uint inflictorObjNameOrLabel;
		const bool damagedIsActivator;

		uint currentHitCount;

	public:
		CndProjHitCount(const ConditionParent& parent,
						const uint damagedObjNameOrLabel,
						const DamagedSurface targetSurface,
						const DamageType damageType,
						const uint targetHitCount,
						const uint inflictorObjNameOrLabel,
						const bool damagedIsActivator);
		~CndProjHitCount();
		void Register();
		void Unregister();
		bool Matches(const IObjRW* damagedObject, const DamageList* damageList, const DamagedSurface damagedSurface);
	};

	namespace Hooks
	{
		namespace CndProjHitCount
		{
			void __stdcall ShipAndSolarEquipDamage(const IObjRW* damagedObject, const CEquip* hitEquip, const float& incomingDamage, const DamageList* damageList);
			void __stdcall ShipAndSolarColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList);
			void __stdcall ShipAndSolarHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList);
			void __stdcall ShipAndSolarShieldDamage(const IObjRW* damagedObject, const CEShield* hitShield, const float& incomingDamage, const DamageList* damageList);
		}
	}
}