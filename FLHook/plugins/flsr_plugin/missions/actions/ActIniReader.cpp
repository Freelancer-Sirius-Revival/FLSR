#include "ActIniReader.h"
#include "ActActMsn.h"
#include "ActActMsnTrig.h"
#include "ActActTrig.h"
#include "ActAddCargo.h"
#include "ActAddLabel.h"
#include "ActAdjAcct.h"
#include "ActAdjRep.h"
#include "ActCloak.h"
#include "ActDebugMsg.h"
#include "ActDestroy.h"
#include "ActDisplayMsg.h"
#include "ActDockInstant.h"
#include "ActEtherComm.h"
#include "ActGiveObjList.h"
#include "ActInvulnerable.h"
#include "ActLeaveGroup.h"
#include "ActLeaveMsn.h"
#include "ActLightFuse.h"
#include "ActMark.h"
#include "ActNNPath.h"
#include "ActPlayMusic.h"
#include "ActPlayNN.h"
#include "ActPlaySoundEffect.h"
#include "ActPopUpDialog.h"
#include "ActRelocate.h"
#include "ActRemoveCargo.h"
#include "ActRemoveLabel.h"
#include "ActSendComm.h"
#include "ActSetDockState.h"
#include "ActSetLifeTime.h"
#include "ActSetMsnResult.h"
#include "ActSetNNObj.h"
#include "ActSetShipAndLoadout.h"
#include "ActSetVibe.h"
#include "ActSpawnFormation.h"
#include "ActSpawnShip.h"
#include "ActSpawnSolar.h"
#include "ActStartDialog.h"
#include "ActTerminateMsn.h"
#include "ActUnlightFuse.h"

namespace Missions
{
	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void PrintErrorToConsole(const INI_Reader& ini, const uint argNum, const std::wstring& error)
	{
		// Using ini.get_line_num is inaccurate at times. That is why the offset is used.
		// -2 to the offset to compensate LR+CR which we have already passed most of the time.
		ConPrint(L"ERROR: " + stows(ini.get_file_name()) + L", Offset " + std::to_wstring(ini.tell() - 2) + L", Arg " + std::to_wstring(argNum + 1) + L": " + error + L"\n");
	}

	static ActActMsn* ReadActActMsn(INI_Reader& ini)
	{
		ActActMsn action;

		uint argNum = 0;
		action.missionId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.missionId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No mission nickname. Aborting!");
			return nullptr;
		}
		argNum++;

		for (const auto maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "all")
			{
				action.playerLabelsToTransfer.clear();
				action.playerLabelsToTransfer.insert(ActActMsnAllPlayerLabels);
				break;
			}
			const auto& label = CreateIdOrNull(value.c_str());
			if (label == 0)
				PrintErrorToConsole(ini, argNum, L"Invalid player label. Ignoring.");
			else
				action.playerLabelsToTransfer.insert(label);
		}

		return new ActActMsn(action);
	}

	static ActActMsnTrig* ReadActActMsnTrig(INI_Reader& ini, const bool activate)
	{
		ActActMsnTrig action;
		action.activate = activate;

		uint argNum = 0;
		action.missionId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.missionId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No mission nickname. Aborting!");
			return nullptr;
		}
		argNum++;

		for (const auto maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			if (argNum % 2 == 1)
			{
				ActActTrigEntry entry;
				entry.triggerId = CreateIdOrNull(ini.get_value_string(argNum));
				if (entry.triggerId == 0)
				{
					PrintErrorToConsole(ini, argNum, L"Invalid trigger nickname. Ignoring.");
					argNum++; // Skip reading the probability after.
				}
				else
					action.triggers.push_back(entry);
			}
			else
			{
				const float value = ini.get_value_float(argNum);
				if (value <= 0.0f)
					PrintErrorToConsole(ini, argNum, L"Probability is 0 or negative. Ignoring.");
				action.triggers.back().probability = std::max<float>(0.0f, value);
			}
		}

		return new ActActMsnTrig(action);
	}

	static ActActTrig* ReadActActTrig(INI_Reader& ini, const bool activate)
	{
		ActActTrig action;
		action.activate = activate;

		uint argNum = 0;
		for (const auto maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			if (argNum % 2 == 0)
			{
				ActActTrigEntry entry;
				entry.triggerId = CreateIdOrNull(ini.get_value_string(argNum));
				if (entry.triggerId == 0)
				{
					PrintErrorToConsole(ini, argNum, L"Invalid trigger nickname. Ignoring.");
					argNum++; // Skip reading the probability after.
				}
				else
					action.triggers.push_back(entry);
			}
			else
			{
				const float value = ini.get_value_float(argNum);
				if (value <= 0.0f)
					PrintErrorToConsole(ini, argNum, L"Probability is 0 or negative. Ignoring.");
				action.triggers.back().probability = std::max<float>(0.0f, value);
			}
		}

		return new ActActTrig(action);
	}

	static ActActTrig* ReadActActTrigBranch(INI_Reader& ini, const bool activate)
	{
		ActActTrig action;
		action.branching = true;
		action.activate = activate;

		uint argNum = 0;
		for (const auto maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			if (argNum % 2 == 0)
			{
				ActActTrigEntry entry;
				entry.triggerId = CreateIdOrNull(ini.get_value_string(argNum));
				if (entry.triggerId == 0)
				{
					PrintErrorToConsole(ini, argNum, L"Invalid trigger nickname. Ignoring.");
					argNum++; // Skip reading the probability after.
				}
				else
					action.triggers.push_back(entry);
			}
			else
			{
				const float value = ini.get_value_float(argNum);
				if (value <= 0.0f)
					PrintErrorToConsole(ini, argNum, L"Probability is 0 or negative. Ignoring.");
				action.triggers.back().probability = std::max<float>(0.0f, value);
			}
		}

		return new ActActTrig(action);
	}

	static ActAddCargo* ReadActAddCargo(INI_Reader& ini)
	{
		ActAddCargo action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.itemId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No item nickname. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const int& count = ini.get_value_int(argNum);
			if (count <= 0)
				PrintErrorToConsole(ini, argNum, L"Count is less than 1. Defaulting to " + std::to_wstring(action.count) + L".");
			else
				action.count = count;
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
			action.missionFlagged = ini.get_value_bool(argNum);

		return new ActAddCargo(action);
	}

	static ActAddLabel* ReadActAddLabel(INI_Reader& ini)
	{
		ActAddLabel action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No new label. Aborting!");
			return nullptr;
		}

		return new ActAddLabel(action);
	}

	static ActAdjAcct* ReadActAdjAcct(INI_Reader& ini)
	{
		ActAdjAcct action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.cash = ini.get_value_int(argNum);
		if (action.cash == 0)
			PrintErrorToConsole(ini, argNum, L"No cash change. Ignoring.");

		return new ActAdjAcct(action);
	}

	static ActAdjRep* ReadActAdjRep(INI_Reader& ini)
	{
		ActAdjRep action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		const std::string faction = ini.get_value_string(argNum);
		if (faction.empty())
		{
			PrintErrorToConsole(ini, argNum, L"No faction. Aborting!");
			return nullptr;
		}
		pub::Reputation::GetReputationGroup(action.groupId, faction.c_str());
		if (action.groupId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"Invalid faction. Aborting!");
			return nullptr;
		}
		argNum++;

		if (argNum >= ini.get_num_parameters())
		{
			PrintErrorToConsole(ini, argNum, L"Reputation change missing. Aborting!");
			return nullptr;
		}

		const auto& val = ToLower(ini.get_value_string(argNum));
		if (val == "objectdestruction")
			action.reason = Empathies::ReputationChangeReason::ObjectDestruction;
		else if (val == "missionsuccess")
			action.reason = Empathies::ReputationChangeReason::MissionSuccess;
		else if (val == "missionfailure")
			action.reason = Empathies::ReputationChangeReason::MissionFailure;
		else if (val == "missionabortion")
			action.reason = Empathies::ReputationChangeReason::MissionAbortion;
		else
		{
			action.change = ini.get_value_float(argNum);
			if (action.change == 0.0f)
				PrintErrorToConsole(ini, argNum, L"No reputation change. Ignoring.");
		}
		
		return new ActAdjRep(action);
	}

	static ActCloak* ReadActCloak(INI_Reader& ini)
	{
		ActCloak action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			action.cloaked = ini.get_value_bool(argNum);

		return new ActCloak(action);
	}

	static ActDebugMsg* ReadActDebugMsg(INI_Reader& ini)
	{
		ActDebugMsg action;

		uint argNum = 0;
		action.message = ini.get_value_string(argNum);
		if (action.message.empty())
		{
			PrintErrorToConsole(ini, argNum, L"No message. Aborting!");
			return nullptr;
		}

		return new ActDebugMsg(action);
	}

	static ActDestroy* ReadActDestroy(INI_Reader& ini)
	{
		ActDestroy action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& val = ToLower(ini.get_value_string(argNum));
			if (val == "explode")
				action.destroyType = DestroyType::EXPLODE;
			else if (val == "silent")
				action.destroyType = DestroyType::VANISH;
			else
				PrintErrorToConsole(ini, argNum, L"Invalid destruction type. Defaulting to Silent.");
		}

		return new ActDestroy(action);
	}

	static ActDisplayMsg* ReadActDisplayMsg(INI_Reader& ini)
	{
		ActDisplayMsg action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.stringId = ini.get_value_int(argNum);
		if (action.stringId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No resource text id. Aborting!");
			return nullptr;
		}

		return new ActDisplayMsg(action);
	}

	static ActDockInstant* ReadActDockInstant(INI_Reader& ini)
	{
		ActDockInstant action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.targetObjName = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.targetObjName == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No dock target obj name. Aborting!");
			return nullptr;
		}
		argNum++;

		action.dockHardpoint = ini.get_value_string(argNum);
		if (action.dockHardpoint.empty())
		{
			PrintErrorToConsole(ini, argNum, L"No dock target hardpoint. Aborting!");
			return nullptr;
		}

		return new ActDockInstant(action);
	}

	static ActEtherComm* ReadActEtherComm(INI_Reader& ini)
	{
		ActEtherComm action;

		uint argNum = 0;
		action.id = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.id == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No comm id. Aborting!");
			return nullptr;
		}
		argNum++;

		action.receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.receiverObjNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No receiver target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.senderVoiceId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.senderVoiceId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No sender voice id. Aborting!");
			return nullptr;
		}
		argNum++;

		while (!ini.is_value_empty(argNum))
		{
			const char* val = ini.get_value_string(argNum);
			// Make sure we do not go beyond the following numeric value.
			char* end;
			strtol(val, &end, 10);
			if (end != val)
				break;
			action.lines.push_back(CreateIdOrNull(val));
			argNum++;
		}
		if (action.lines.empty())
		{
			PrintErrorToConsole(ini, argNum, L"No voice lines. Aborting!");
			return nullptr;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& delay = ini.get_value_float(argNum);
			if (delay < 0.0f)
				PrintErrorToConsole(ini, argNum, L"Delay is below 0. Defaulting to " + std::to_wstring(action.delay) + L".");
			else
				action.delay = delay;
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
			action.global = ini.get_value_bool(argNum++);

		action.senderIdsName = ini.get_value_int(argNum++);
		action.costume.head = CreateIdOrNull(ini.get_value_string(argNum++));
		action.costume.body = CreateIdOrNull(ini.get_value_string(argNum++));
		byte count = 0;
		while (count < 8)
		{
			const auto val = ini.get_value_string(argNum++);
			if (strlen(val) == 0)
				break;
			action.costume.accessory[count++] = CreateIdOrNull(val);
		}
		action.costume.accessories = count;

		return new ActEtherComm(action);
	}

	static ActGiveObjList* ReadActGiveObjList(INI_Reader& ini)
	{
		ActGiveObjList action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.objectivesId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objectivesId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No objectives list name. Aborting!");
			return nullptr;
		}

		return new ActGiveObjList(action);
	}

	static ActInvulnerable* ReadActInvulnerable(INI_Reader& ini)
	{
		ActInvulnerable action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			action.preventNonPlayerDamage = ini.get_value_bool(argNum++);

		if (ini.get_num_parameters() > argNum)
			action.preventPlayerDamage = ini.get_value_bool(argNum++);

		if (ini.get_num_parameters() > argNum)
		{
			const auto& maxHpLossPercentage = ini.get_value_float(argNum);
			if (maxHpLossPercentage < 0.0f)
				PrintErrorToConsole(ini, argNum, L"Max HP loss percentage is below 0. Defaulting to " + std::to_wstring(action.maxHpLossPercentage) + L".");
			else if (maxHpLossPercentage >= 1.0f)
				PrintErrorToConsole(ini, argNum, L"Max HP loss percentage is exactly 1 or above. Defaulting to " + std::to_wstring(action.maxHpLossPercentage) + L".");
			else
				action.maxHpLossPercentage = maxHpLossPercentage;
		}

		return new ActInvulnerable(action);
	}

	static ActLeaveGroup* ReadActLeaveGroup(INI_Reader& ini)
	{
		ActLeaveGroup action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}

		return new ActLeaveGroup(action);
	}

	static ActLeaveMsn* ReadActLeaveMsn(INI_Reader& ini)
	{
		ActLeaveMsn action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& val = ToLower(ini.get_value_string(argNum));
			if (val == "success")
				action.leaveType = LeaveMsnType::Success;
			else if (val == "failure")
				action.leaveType = LeaveMsnType::Failure;
			else
				PrintErrorToConsole(ini, argNum, L"Invalid mission leave type. Defaulting to Silent.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			if (action.leaveType != LeaveMsnType::Failure)
				PrintErrorToConsole(ini, argNum, L"Failure resource id given, but leave type is not Failure. Ignoring.");
			else
			{
				action.failureStringId = ini.get_value_int(argNum);
				if (action.failureStringId == 0)
					PrintErrorToConsole(ini, argNum, L"No failure resource id. Ignoring.");
			}
		}

		return new ActLeaveMsn(action);
	}

	static ActLightFuse* ReadActLightFuse(INI_Reader& ini)
	{
		ActLightFuse action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.fuse = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.fuse == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No fuse name. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& timeOffset = ini.get_value_float(argNum);
			if (timeOffset < 0.0f)
				PrintErrorToConsole(ini, argNum, L"Time offset is below 0. Defaulting to " + std::to_wstring(action.timeOffset) + L".");
			else if (timeOffset > 1.0f)
				PrintErrorToConsole(ini, argNum, L"Time offset is above 1. Defaulting to " + std::to_wstring(action.timeOffset) + L".");
			else
				action.timeOffset = timeOffset;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			action.lifetimeOverride = ini.get_value_float(argNum);

		return new ActLightFuse(action);
	}

	static ActMark* ReadActMark(INI_Reader& ini)
	{
		ActMark action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.targetObjNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.targetObjNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No mark target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			action.marked = ini.get_value_bool(argNum);

		return new ActMark(action);
	}

	static ActNNPath* ReadActNNPath(INI_Reader& ini)
	{
		ActNNPath action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.message = CreateIdOrNull(ini.get_value_string(argNum++));

		if (ini.get_num_parameters() > argNum)
		{
			if (ini.get_num_parameters() >= argNum + 4)
			{
				action.systemId = CreateIdOrNull(ini.get_value_string(argNum));
				if (action.systemId == 0)
				{
					PrintErrorToConsole(ini, argNum, L"No target system. Aborting!");
					return nullptr;
				}
				argNum++;
				action.position.x = ini.get_value_float(argNum++);
				action.position.y = ini.get_value_float(argNum++);
				action.position.z = ini.get_value_float(argNum++);

				if (ini.get_num_parameters() > argNum)
					action.bestRoute = ini.get_value_bool(argNum++);
				if (ini.get_num_parameters() > argNum)
					action.targetObjName = CreateIdOrNull(ini.get_value_string(argNum++));
			}
			else
				PrintErrorToConsole(ini, argNum, L"No target system and positions given. Defaulting to none.");
		}

		return new ActNNPath(action);
	}

	static ActPlayMusic* ReadActPlayMusic(INI_Reader& ini)
	{
		ActPlayMusic action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.music.spaceMusic = ToLower(ini.get_value_string(argNum)) != "default" ? CreateIdOrNull(ini.get_value_string(argNum)) : 0;
		argNum++;

		action.music.dangerMusic = ToLower(ini.get_value_string(argNum)) != "default" ? CreateIdOrNull(ini.get_value_string(argNum)) : 0;
		argNum++;

		action.music.battleMusic = ToLower(ini.get_value_string(argNum)) != "default" ? CreateIdOrNull(ini.get_value_string(argNum)) : 0;
		argNum++;

		action.music.overrideMusic = ToLower(ini.get_value_string(argNum)) != "none" ? CreateIdOrNull(ini.get_value_string(argNum)) : 0;
		argNum++;

		const auto& crossFadeDurationInS = ini.get_value_float(argNum);
		if (crossFadeDurationInS < 0.0f)
			PrintErrorToConsole(ini, argNum, L"Music cross fade duration is below 0. Defaulting to " + std::to_wstring(action.music.crossFadeDurationInS) + L".");
		else
			action.music.crossFadeDurationInS = crossFadeDurationInS;
		argNum++;

		if (ini.get_num_parameters() > 6)
			action.music.playOnce = ini.get_value_bool(6);

		return new ActPlayMusic(action);
	}

	static ActPlayNN* ReadActPlayNN(INI_Reader& ini)
	{
		ActPlayNN action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& soundId = CreateIdOrNull(ini.get_value_string(argNum));
			if (soundId == 0)
				PrintErrorToConsole(ini, argNum, L"Empty voice line. Skipping.");
			else
				action.soundIds.push_back(soundId);
			argNum++;
		}
		if (action.soundIds.empty())
		{
			PrintErrorToConsole(ini, argNum, L"No voice lines. Aborting!");
			return nullptr;
		}

		return new ActPlayNN(action);
	}

	static ActPlaySoundEffect* ReadActPlaySoundEffect(INI_Reader& ini)
	{
		ActPlaySoundEffect action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.soundId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.soundId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"Sound is missing. Aborting!");
			return nullptr;
		}

		return new ActPlaySoundEffect(action);
	}

	static ActPopUpDialog* ReadActPopUpDialog(INI_Reader& ini)
	{
		ActPopUpDialog action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.headingId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.headingId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No heading resource ID. Aborting!");
			return nullptr;
		}
		argNum++;

		action.messageId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.messageId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No message resource ID. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& button = ToLower(ini.get_value_string(argNum));
			if (button == "close")
				action.buttons |= PopupDialogButton::CENTER_OK;
			else if (button == "yes")
				action.buttons |= PopupDialogButton::LEFT_YES;
			else if (button == "no")
				action.buttons |= PopupDialogButton::CENTER_NO;
			else if (button == "later")
				action.buttons |= PopupDialogButton::RIGHT_LATER;
			else
				PrintErrorToConsole(ini, argNum, L"No valid button type. Skipping.");
			argNum++;
		}		
		if (action.buttons == 0)
			PrintErrorToConsole(ini, argNum, L"No button types given. Defaulting to OK.");

		return new ActPopUpDialog(action);
	}

	static ActRelocate* ReadActRelocate(INI_Reader& ini)
	{
		ActRelocate action;

		uint argNum = 0;
		action.objName = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objName == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() >= argNum + 4)
		{
			action.systemId = CreateIdOrNull(ini.get_value_string(argNum));
			if (action.systemId == 0)
			{
				PrintErrorToConsole(ini, argNum, L"No target system. Aborting!");
				return nullptr;
			}
			argNum++;
			action.position.x = ini.get_value_float(argNum++);
			action.position.y = ini.get_value_float(argNum++);
			action.position.z = ini.get_value_float(argNum++);
		}
		else
		{
			PrintErrorToConsole(ini, argNum, L"No target system and positions given. Aborting!");
			return nullptr;
		}

		if (ini.get_num_parameters() > argNum)
		{
			if (argNum + 3 < ini.get_num_parameters())
			{
				PrintErrorToConsole(ini, argNum, L"Rotation values are incomplete. Skipping!");
				return nullptr;
			}
			Vector rotation;
			rotation.x = ini.get_value_float(argNum++);
			rotation.y = ini.get_value_float(argNum++);
			rotation.z = ini.get_value_float(argNum++);
			action.orientation = EulerMatrix(rotation);
		}

		return new ActRelocate(action);
	}

	static ActRemoveCargo* ReadActRemoveCargo(INI_Reader& ini)
	{
		ActRemoveCargo action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.itemId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.itemId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No item nickname. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& count = ini.get_value_int(argNum);
			if (count < 0)
				PrintErrorToConsole(ini, argNum, L"Count is below 0. Defaulting to " + std::to_wstring(action.count) + L".");
			else
				action.count = count;
		}

		return new ActRemoveCargo(action);
	}

	static ActRemoveLabel* ReadActRemoveLabel(INI_Reader& ini)
	{
		ActRemoveLabel action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No label. Aborting!");
			return nullptr;
		}

		return new ActRemoveLabel(action);
	}

	static ActSendComm* ReadActSendComm(INI_Reader& ini)
	{
		ActSendComm action;

		uint argNum = 0;
		action.id = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.id == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No comm id. Aborting!");
			return nullptr;
		}
		argNum++;

		action.receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.receiverObjNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No receiver target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.senderObjName = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.senderObjName == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No sender obj name. Aborting!");
			return nullptr;
		}
		argNum++;

		while (!ini.is_value_empty(argNum))
		{
			const char* val = ini.get_value_string(argNum);
			// Make sure we do not go beyond the following numeric value.
			char* end;
			strtol(val, &end, 10);
			if (end != val)
				break;
			action.lines.push_back(CreateIdOrNull(val));
			argNum++;
		}
		if (action.lines.empty())
		{
			PrintErrorToConsole(ini, argNum, L"No voice lines. Aborting!");
			return nullptr;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& delay = ini.get_value_float(argNum);
			if (delay < 0.0f)
				PrintErrorToConsole(ini, argNum, L"Delay is below 0. Defaulting to " + std::to_wstring(action.delay) + L".");
			else
				action.delay = delay;
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
			action.global = ini.get_value_bool(argNum++);

		return new ActSendComm(action);
	}

	static ActSetDockState* ReadActSetDockState(INI_Reader& ini)
	{
		ActSetDockState action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.dockHardpoint = ToLower(ini.get_value_string(argNum));
		if (action.dockHardpoint.empty())
		{
			PrintErrorToConsole(ini, argNum, L"No hardpoint. Aborting!");
			return nullptr;
		}
		argNum++;

		const auto& state = ToLower(ini.get_value_string(argNum));
		if (state == "opened")
			action.opened = true;
		else if (state == "closed")
			action.opened = false;
		else
			PrintErrorToConsole(ini, argNum, L"No valid dock state. Defaulting to Opened.");

		return new ActSetDockState(action);
	}

	static ActSetLifeTime* ReadActSetLifeTime(INI_Reader& ini)
	{
		ActSetLifeTime action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.lifeTime = ini.get_value_float(argNum);
		
		return new ActSetLifeTime(action);
	}

	static ActSetMsnResult* ReadActSetMsnResult(INI_Reader& ini)
	{
		ActSetMsnResult action;

		uint argNum = 0;
		const auto& result = ToLower(ini.get_value_string(argNum));
		if (result == "success")
			action.result = Mission::MissionResult::Success;
		else if (result == "failure")
			action.result = Mission::MissionResult::Failure;
		else
		{
			PrintErrorToConsole(ini, argNum, L"No valid mission result. Aborting!");
			return nullptr;
		}

		return new ActSetMsnResult(action);
	}

	static ActSetNNObj* ReadActSetNNObj(INI_Reader& ini)
	{
		ActSetNNObj action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.message = FmtStr(ini.get_value_int(argNum++), 0);

		if (ini.get_num_parameters() > argNum)
		{
			if (ini.get_num_parameters() >= argNum + 4)
			{
				action.systemId = CreateIdOrNull(ini.get_value_string(argNum));
				if (action.systemId == 0)
				{
					PrintErrorToConsole(ini, argNum, L"No target system. Aborting!");
					return nullptr;
				}
				argNum++;
				action.position.x = ini.get_value_float(argNum++);
				action.position.y = ini.get_value_float(argNum++);
				action.position.z = ini.get_value_float(argNum++);

				if (ini.get_num_parameters() > argNum)
					action.bestRoute = ini.get_value_bool(argNum++);
				if (ini.get_num_parameters() > argNum)
					action.targetObjName = CreateIdOrNull(ini.get_value_string(argNum++));
			}
			else
				PrintErrorToConsole(ini, argNum, L"No target system and positions given. Defaulting to none.");
		}

		return new ActSetNNObj(action);
	}

	static ActSetShipAndLoadout* ReadActSetShipAndLoadout(INI_Reader& ini)
	{
		ActSetShipAndLoadout action;

		uint argNum = 0;
		action.label = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.label == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.shipArchetypeId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.shipArchetypeId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No shiparch. Aborting!");
			return nullptr;
		}
		argNum++;

		action.loadoutId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.loadoutId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No loadout. Aborting!");
			return nullptr;
		}

		return new ActSetShipAndLoadout(action);
	}

	static ActSetVibe* ReadActSetVibe(INI_Reader& ini)
	{
		ActSetVibe action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.targetObjNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.targetObjNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		const auto& reputation = ini.get_value_float(argNum);
		if (reputation < -1.0f)
			PrintErrorToConsole(ini, argNum, L"Reputation is below -1. Defaulting to " + std::to_wstring(action.reputation) + L".");
		else if (reputation > 1.0f)
			PrintErrorToConsole(ini, argNum, L"Reputation is above 1. Defaulting to " + std::to_wstring(action.reputation) + L".");
		else
			action.reputation = reputation;

		return new ActSetVibe(action);
	}

	static ActSpawnFormation* ReadActSpawnFormation(INI_Reader& ini)
	{
		ActSpawnFormation action;

		uint argNum = 0;
		action.msnFormationId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.msnFormationId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No MSN Formation. Aborting!");
			return nullptr;
		}
		argNum++;

		const auto objectivesName = ToLower(ini.get_value_string(argNum));
		action.objectivesId = objectivesName == "no_ol" ? 0 : CreateID(objectivesName.c_str());
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			if (argNum + 3 < ini.get_num_parameters())
			{
				PrintErrorToConsole(ini, argNum, L"Position values are incomplete. Skipping!");
				return nullptr;
			}
			action.position.x = ini.get_value_float(argNum++);
			action.position.y = ini.get_value_float(argNum++);
			action.position.z = ini.get_value_float(argNum++);
		}

		if (ini.get_num_parameters() > argNum)
		{
			if (argNum + 3 < ini.get_num_parameters())
			{
				PrintErrorToConsole(ini, argNum, L"Rotation values are incomplete. Skipping!");
				return nullptr;
			}
			action.rotation.x = ini.get_value_float(argNum++);
			action.rotation.y = ini.get_value_float(argNum++);
			action.rotation.z = ini.get_value_float(argNum++);
		}

		return new ActSpawnFormation(action);
	}

	static ActSpawnShip* ReadActSpawnShip(INI_Reader& ini)
	{
		ActSpawnShip action;

		uint argNum = 0;
		action.msnNpcId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.msnNpcId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No MSN NPC. Aborting!");
			return nullptr;
		}
		argNum++;

		const auto objectivesName = ToLower(ini.get_value_string(argNum));
		action.objectivesId = objectivesName == "no_ol" ? 0 : CreateID(objectivesName.c_str());
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			if (argNum + 3 < ini.get_num_parameters())
			{
				PrintErrorToConsole(ini, argNum, L"Position values are incomplete. Skipping!");
				return nullptr;
			}
			action.position.x = ini.get_value_float(argNum++);
			action.position.y = ini.get_value_float(argNum++);
			action.position.z = ini.get_value_float(argNum++);
		}

		if (ini.get_num_parameters() > argNum)
		{
			if (argNum + 3 < ini.get_num_parameters())
			{
				PrintErrorToConsole(ini, argNum, L"Rotation values are incomplete. Skipping!");
				return nullptr;
			}
			Vector rotation;
			rotation.x = ini.get_value_float(argNum++);
			rotation.y = ini.get_value_float(argNum++);
			rotation.z = ini.get_value_float(argNum++);
			action.orientation = EulerMatrix(rotation);
		}

		return new ActSpawnShip(action);
	}

	static ActSpawnSolar* ReadActSpawnSolar(INI_Reader& ini)
	{
		ActSpawnSolar action;

		uint argNum = 0;
		action.solarId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.solarId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No MSN Solar. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			if (argNum + 3 < ini.get_num_parameters())
			{
				PrintErrorToConsole(ini, argNum, L"Position values are incomplete. Skipping!");
				return nullptr;
			}
			action.position.x = ini.get_value_float(argNum++);
			action.position.y = ini.get_value_float(argNum++);
			action.position.z = ini.get_value_float(argNum++);
		}

		if (ini.get_num_parameters() > argNum)
		{
			if (argNum + 3 < ini.get_num_parameters())
			{
				PrintErrorToConsole(ini, argNum, L"Rotation values are incomplete. Skipping!");
				return nullptr;
			}
			Vector rotation;
			rotation.x = ini.get_value_float(argNum++);
			rotation.y = ini.get_value_float(argNum++);
			rotation.z = ini.get_value_float(argNum++);
			action.orientation = EulerMatrix(rotation);
		}

		return new ActSpawnSolar(action);
	}

	static ActStartDialog* ReadActStartDialog(INI_Reader& ini)
	{
		ActStartDialog action;

		uint argNum = 0;
		action.dialogId = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.dialogId == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No dialog name. Aborting!");
			return nullptr;
		}

		return new ActStartDialog(action);
	}

	static ActUnlightFuse* ReadActUnlightFuse(INI_Reader& ini)
	{
		ActUnlightFuse action;

		uint argNum = 0;
		action.objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.objNameOrLabel == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		action.fuse = CreateIdOrNull(ini.get_value_string(argNum));
		if (action.fuse == 0)
		{
			PrintErrorToConsole(ini, argNum, L"No fuse name. Aborting!");
			return nullptr;
		}

		return new ActUnlightFuse(action);
	}

	Action* TryReadActionFromIni(INI_Reader& ini)
	{
		if (ini.is_value("Act_ActMsn"))
			return ReadActActMsn(ini);
		
		if (ini.is_value("Act_ActMsnTrig"))
			return ReadActActMsnTrig(ini, true);
		
		if (ini.is_value("Act_ActTrig"))
			return ReadActActTrig(ini, true);

		if (ini.is_value("Act_ActTrigBranch"))
			return ReadActActTrigBranch(ini, true);

		if (ini.is_value("Act_AddCargo"))
			return ReadActAddCargo(ini);

		if (ini.is_value("Act_AddLabel"))
			return ReadActAddLabel(ini);

		if (ini.is_value("Act_AdjAcct"))
			return ReadActAdjAcct(ini);

		if (ini.is_value("Act_AdjRep"))
			return ReadActAdjRep(ini);

		if (ini.is_value("Act_Cloak"))
			return ReadActCloak(ini);

		if (ini.is_value("Act_DeactMsnTrig"))
			return ReadActActMsnTrig(ini, false);

		if (ini.is_value("Act_DeactTrig"))
			return ReadActActTrig(ini, false);

		if (ini.is_value("Act_DeactTrigBranch"))
			return ReadActActTrigBranch(ini, false);

		if (ini.is_value("Act_DebugMsg"))
			return ReadActDebugMsg(ini);

		if (ini.is_value("Act_Destroy"))
			return ReadActDestroy(ini);

		if (ini.is_value("Act_DisplayMsg"))
			return ReadActDisplayMsg(ini);

		if (ini.is_value("Act_DockInstant"))
			return ReadActDockInstant(ini);

		if (ini.is_value("Act_EtherComm"))
			return ReadActEtherComm(ini);

		if (ini.is_value("Act_GiveObjList"))
			return ReadActGiveObjList(ini);

		if (ini.is_value("Act_Invulnerable"))
			return ReadActInvulnerable(ini);

		if (ini.is_value("Act_LeaveGroup"))
			return ReadActLeaveGroup(ini);

		if (ini.is_value("Act_LeaveMsn"))
			return ReadActLeaveMsn(ini);

		if (ini.is_value("Act_LightFuse"))
			return ReadActLightFuse(ini);

		if (ini.is_value("Act_Mark"))
			return ReadActMark(ini);

		if (ini.is_value("Act_NNPath"))
			return ReadActNNPath(ini);

		if (ini.is_value("Act_PlayMusic"))
			return ReadActPlayMusic(ini);

		if (ini.is_value("Act_PlayNN"))
			return ReadActPlayNN(ini);

		if (ini.is_value("Act_PlaySoundEffect"))
			return ReadActPlaySoundEffect(ini);

		if (ini.is_value("Act_PopUpDialog"))
			return ReadActPopUpDialog(ini);

		if (ini.is_value("Act_Relocate"))
			return ReadActRelocate(ini);

		if (ini.is_value("Act_RemoveCargo"))
			return ReadActRemoveCargo(ini);

		if (ini.is_value("Act_RemoveLabel"))
			return ReadActRemoveLabel(ini);

		if (ini.is_value("Act_SendComm"))
			return ReadActSendComm(ini);

		if (ini.is_value("Act_SetDockState"))
			return ReadActSetDockState(ini);

		if (ini.is_value("Act_SetLifeTime"))
			return ReadActSetLifeTime(ini);

		if (ini.is_value("Act_SetMsnResult"))
			return ReadActSetMsnResult(ini);

		if (ini.is_value("Act_SetNNObj"))
			return ReadActSetNNObj(ini);

		if (ini.is_value("Act_SetShipAndLoadout"))
			return ReadActSetShipAndLoadout(ini);

		if (ini.is_value("Act_SetVibe"))
			return ReadActSetVibe(ini);

		if (ini.is_value("Act_SpawnFormation"))
			return ReadActSpawnFormation(ini);

		if (ini.is_value("Act_SpawnShip"))
			return ReadActSpawnShip(ini);

		if (ini.is_value("Act_SpawnSolar"))
			return ReadActSpawnSolar(ini);

		if (ini.is_value("Act_StartDialog"))
			return ReadActStartDialog(ini);

		if (ini.is_value("Act_TerminateMsn"))
			return new ActTerminateMsn();

		if (ini.is_value("Act_UnlightFuse"))
			return ReadActUnlightFuse(ini);
		
		return nullptr;
	}
}