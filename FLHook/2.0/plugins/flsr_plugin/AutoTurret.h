#pragma once
#include <FLHook.h>

namespace AutoTurret
{
	void __fastcall CGuidedInit(CGuided* cguided, CGuided::CreateParms& param);
	void __stdcall Elapse_Time_AFTER(float seconds);
}