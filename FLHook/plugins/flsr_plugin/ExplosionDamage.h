#pragma once
#include <FLHook.h>

namespace ExplosionDamage
{
	bool __stdcall ExplosionHit(IObjRW* iobj, ExplosionDamageEvent* explosion, DamageList* dmg);
}