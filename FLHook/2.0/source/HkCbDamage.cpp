#include "hook.h"

FARPROC fpOldExplosionHit;

bool __stdcall ExplosionHit(IObjRW* iobj, ExplosionDamageEvent* explosion, DamageList* dmg)
{
	CALL_PLUGINS(PLUGIN_ExplosionHit, bool, __stdcall, (IObjRW * iobj, ExplosionDamageEvent * explosion, DamageList * dmg), (iobj, explosion, dmg));
	return true;
}

__declspec(naked) void HookExplosionHitNaked()
{
	__asm {
		push ecx
		push[esp + 0xC]
		push[esp + 0xC]
		push ecx
		call ExplosionHit
		pop ecx
		ret 0x8
	}
}

/**************************************************************************************************************
Called when ship was damaged
**************************************************************************************************************/

FARPROC ApplyShipDamageListOrigFunc;

void __stdcall SetDamageToOne(IObjRW* ship, DamageList* dmg, uint source)
{
	if (source != 0x6cfe254) // only work when dmg source is a damage fuse
	{
		return;
	}
	for (auto& dmgEntry : dmg->damageentries)
	{
		if (dmgEntry.subobj == 1 && dmgEntry.health <= 0.0f)
		{
			dmgEntry.health = 0.1f;
			CShip* cship = (CShip*)ship->cobj;
			cship->isUndergoingDeathFuse = true;
		}
	}
}

__declspec(naked) void ApplyShipDamageListNaked()
{
	__asm {
		push ecx
		push[esp + 0x4]
		push[esp + 0xC]
		push ecx
		call SetDamageToOne
		pop ecx
		jmp ApplyShipDamageListOrigFunc
	}
}

FARPROC ShipHullDamageOrigFunc;

void __stdcall ShipHullDamage(IObjRW* iobj, float& incDmg, DamageList* dmg)
{
	CSimple* simple = reinterpret_cast<CSimple*>(iobj->cobj);

	CALL_PLUGINS_V(PLUGIN_ShipHullDmg, __stdcall, (IObjRW * iobj, float& incDmg, DamageList * dmg), (iobj, incDmg, dmg));
	if (simple->ownerPlayer)
	{
		ClientInfo[simple->ownerPlayer].dmgLastCause = dmg->damageCause;
	}
}

__declspec(naked) void ShipHullDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0xC]
		lea eax, [esp + 0xC]
		push eax
		push ecx
		call ShipHullDamage
		pop ecx
		jmp[ShipHullDamageOrigFunc]
	}
}

FARPROC ShipEquipDamageOrigFunc;

void __stdcall ShipEquipDamage(IObjRW* iobj, CEquip* equipHit, float& incDmg, DamageList* dmg)
{
	CSimple* simple = reinterpret_cast<CSimple*>(iobj->cobj);

	CALL_PLUGINS_V(PLUGIN_ShipEquipDmg, __stdcall, (IObjRW * iobj, CEquip * equipHit, float& incDmg, DamageList * dmg), (iobj, equipHit, incDmg, dmg));
	if (simple->ownerPlayer)
	{
		ClientInfo[simple->ownerPlayer].dmgLastCause = dmg->damageCause;
	}
}

__declspec(naked) void ShipEquipDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0x10]
		lea eax, [esp + 0x10]
		push eax
		push[esp + 0x10]
		push ecx
		call ShipEquipDamage
		pop ecx
		jmp[ShipEquipDamageOrigFunc]
	}
}

FARPROC ShipShieldDamageOrigFunc;

void __stdcall ShipShieldDamage(IObjRW* iobj, CEShield* shieldHit, float& incDmg, DamageList* dmg)
{
	CSimple* simple = reinterpret_cast<CSimple*>(iobj->cobj);

	CALL_PLUGINS_V(PLUGIN_ShipShieldDmg, __stdcall, (IObjRW * iobj, CEShield * shieldHit, float& incDmg, DamageList * dmg), (iobj, shieldHit, incDmg, dmg));
	if (simple->ownerPlayer)
	{
		ClientInfo[simple->ownerPlayer].dmgLastCause = dmg->damageCause;
	}
}

__declspec(naked) void ShipShieldDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0x10]
		lea eax, [esp + 0x10]
		push eax
		push[esp + 0x10]
		push ecx
		call ShipShieldDamage
		pop ecx
		jmp[ShipShieldDamageOrigFunc]
	}
}

FARPROC ShipColGrpDamageOrigFunc;

void __stdcall ShipColGrpDamage(IObjRW* iobj, CArchGroup* colGrpHit, float& incDmg, DamageList* dmg)
{
	CSimple* simple = reinterpret_cast<CSimple*>(iobj->cobj);

	CALL_PLUGINS_V(PLUGIN_ShipColGrpDmg, __stdcall, (IObjRW * iobj, CArchGroup * colGrpHit, float& incDmg, DamageList * dmg), (iobj, colGrpHit, incDmg, dmg));
	if (simple->ownerPlayer)
	{
		ClientInfo[simple->ownerPlayer].dmgLastCause = dmg->damageCause;
	}
}

__declspec(naked) void ShipColGrpDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0x10]
		lea eax, [esp + 0x10]
		push eax
		push[esp + 0x10]
		push ecx
		call ShipColGrpDamage
		pop ecx
		jmp[ShipColGrpDamageOrigFunc]
	}
}


FARPROC SolarHullDamageOrigFunc;

void __stdcall SolarHullDamage(IObjRW* iobj, float& incDmg, DamageList* dmg)
{
	CALL_PLUGINS_V(PLUGIN_SolarHullDmg, __stdcall, (IObjRW * iobj, float& incDmg, DamageList * dmg), (iobj, incDmg, dmg));
}

__declspec(naked) void SolarHullDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0xC]
		lea eax, [esp + 0xC]
		push eax
		push ecx
		call SolarHullDamage
		pop ecx
		jmp[SolarHullDamageOrigFunc]
	}
}

FARPROC SolarEquipDamageOrigFunc;

void __stdcall SolarEquipDamage(IObjRW* iobj, CEquip* equipHit, float& incDmg, DamageList* dmg)
{
	CALL_PLUGINS_V(PLUGIN_SolarEquipDmg, __stdcall, (IObjRW * iobj, CEquip * equipHit, float& incDmg, DamageList * dmg), (iobj, equipHit, incDmg, dmg));
}

__declspec(naked) void SolarEquipDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0x10]
		lea eax, [esp + 0x10]
		push eax
		push[esp + 0x10]
		push ecx
		call SolarEquipDamage
		pop ecx
		jmp[SolarEquipDamageOrigFunc]
	}
}

FARPROC SolarShieldDamageOrigFunc;

void __stdcall SolarShieldDamage(IObjRW* iobj, CEShield* shieldHit, float& incDmg, DamageList* dmg)
{
	CALL_PLUGINS_V(PLUGIN_SolarShieldDmg, __stdcall, (IObjRW * iobj, CEShield * shieldHit, float& incDmg, DamageList * dmg), (iobj, shieldHit, incDmg, dmg));
}

__declspec(naked) void SolarShieldDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0x10]
		lea eax, [esp + 0x10]
		push eax
		push[esp + 0x10]
		push ecx
		call SolarShieldDamage
		pop ecx
		jmp[SolarShieldDamageOrigFunc]
	}
}

FARPROC SolarColGrpDamageOrigFunc;

void __stdcall SolarColGrpDamage(IObjRW* iobj, CArchGroup* colGrpHit, float& incDmg, DamageList* dmg)
{
	CALL_PLUGINS_V(PLUGIN_SolarColGrpDmg, __stdcall, (IObjRW * iobj, CArchGroup * colGrpHit, float& incDmg, DamageList * dmg), (iobj, colGrpHit, incDmg, dmg));
}

__declspec(naked) void SolarColGrpDamageNaked()
{
	__asm {
		push ecx
		push[esp + 0x10]
		lea eax, [esp + 0x10]
		push eax
		push[esp + 0x10]
		push ecx
		call SolarColGrpDamage
		pop ecx
		jmp[SolarColGrpDamageOrigFunc]
	}
}