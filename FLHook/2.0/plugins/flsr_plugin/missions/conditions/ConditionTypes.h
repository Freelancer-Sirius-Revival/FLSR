#pragma once

namespace Missions
{
	enum class ConditionType
	{
		Cnd_BaseEnter,
		Cnd_BaseExit,
		Cnd_CargoScanned,
		Cnd_Cloaked,
		Cnd_CommComplete,
		Cnd_Count,
		Cnd_Destroyed,
		Cnd_DistCircle,
		Cnd_DistShip,
		Cnd_DistVec,
		Cnd_HealthDec,
		Cnd_InTradelane,
		Cnd_JumpInComplete,
		Cnd_LaunchComplete,
		Cnd_LootAcquired,
		Cnd_PlayerManeuver,
		Cnd_PopUpDialog,
		Cnd_ProjHit,
		Cnd_SpaceEnter,
		Cnd_SpaceExit,
		Cnd_SystemEnter,
		Cnd_SystemExit,
		Cnd_TLEntered,
		Cnd_TLExited,
		Cnd_Timer,
		Cnd_True,
		Cnd_WatchTrigger,
		Cnd_WatchVibe
	};
}