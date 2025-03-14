#pragma once
#include <iostream>
#include <vector>
#include "conditions/Conditions.h"

namespace Missions
{
	enum class TriggerAction
	{
		Act_ActTrig,
		Act_AddAmbient,
		Act_AddCargo,
		Act_AddLabel,
		Act_AddRTC,
		Act_AdjAcct,
		Act_AdjHealth,
		Act_CallThorn,
		Act_ChangeState,
		Act_Cloak,
		Act_DeactTrig,
		Act_DebugMsg,
		Act_Destroy,
		Act_DisableEnc,
		Act_DisableFriendlyFire,
		Act_DisableTradelane,
		Act_DockRequest,
		Act_EndMission,
		Act_EnableEnc,
		Act_EnableManeuver,
		Act_EtherComm,
		Act_ForceLand,
		Act_GCSClamp,
		Act_GiveMB,
		Act_GiveNNObjs,
		Act_GiveObjList,
		Act_HostileClamp,
		Act_Invulnerable,
		Act_Jumper,
		Act_LightFuse,
		Act_LockDock,
		Act_LockManeuvers,
		Act_MarkObj,
		Act_MovePlayer,
		Act_NagClamp,
		Act_NagDistLeaving,
		Act_NagDistTowards,
		Act_NagGreet,
		Act_NagOff,
		Act_NNPath,
		Act_NNIds,
		Act_PlayerCanDock,
		Act_PlayerCanTradelane,
		Act_PlayerEnemyClamp,
		Act_PlayerForm,
		Act_PlayMusic,
		Act_PlayNN,
		Act_PlaySoundEffect,
		Act_PilotParams,
		Act_PobjIdle,
		Act_PopUpDialog,
		Act_SetPriority,
		Act_RandomPop,
		Act_RandomPopSphere,
		Act_RelocateForm,
		Act_RelocateShip,
		Act_RemoveAmbient,
		Act_RemoveCargo,
		Act_RemoveLabel,
		Act_RemoveRTC,
		Act_RepChangeRequest,
		Act_RevertCam,
		Act_RpopAttClamp,
		Act_RpopTLAttacksEnabled,
		Act_Save,
		Act_SendComm,
		Act_SetFlee,
		Act_SetInitialPlayerPos,
		Act_SetLifeTime,
		Act_SetNNHidden,
		Act_SetNNObj,
		Act_SetNNState,
		Act_SetOffer,
		Act_SetOrient,
		Act_SetRep,
		Act_SetShipAndLoadout,
		Act_SetTitle,
		Act_SetVibe,
		Act_SetVibeLbl,
		Act_SetVibeLblToShip,
		Act_SetVibeShipToLbl,
		Act_SetVibeOfferBaseHack,
		Act_SpawnFormation,
		Act_SpawnLoot,
		Act_SpawnShip,
		Act_SpawnShipRel,
		Act_SpawnSolar,
		Act_StartDialog,
		Act_StaticCam
	};

	typedef std::pair<TriggerCondition, std::shared_ptr<void>> TriggerArchConditionEntry;
	typedef std::pair<TriggerAction, std::shared_ptr<void>> TriggerArchActionEntry;

	struct TriggerArchetype
	{
		std::string name = "";
		bool active = false;
		bool repeatable = false;
		TriggerArchConditionEntry condition = { TriggerCondition::Cnd_True, nullptr };
		std::vector<TriggerArchActionEntry> actions;
	};
	typedef std::shared_ptr<TriggerArchetype> TriggerArchetypePtr;
}