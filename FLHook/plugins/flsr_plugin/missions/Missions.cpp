#include <regex>
#include <filesystem>
#include "../Empathies.h"
#include "Missions.h"
#include "MissionIniReader.h"
#include "Conditions/CndTrue.h"
#include "Conditions/CndIniReader.h"
#include "Actions/ActDebugMsg.h"
#include "Actions/ActSetShipAndLoadout.h"
#include "Actions/ActActTrig.h"
#include "Actions/ActActMsn.h"
#include "Actions/ActActMsnTrig.h"
#include "Actions/ActSetMsnResult.h"
#include "Actions/ActAddLabel.h"
#include "Actions/ActRemoveLabel.h"
#include "Actions/ActLightFuse.h"
#include "Actions/ActUnlightFuse.h"
#include "Actions/ActSpawnSolar.h"
#include "Actions/ActSpawnShip.h"
#include "Actions/ActSpawnFormation.h"
#include "Actions/ActTerminateMsn.h"
#include "Actions/ActDestroy.h"
#include "Actions/ActPlaySoundEffect.h"
#include "Actions/ActRelocate.h"
#include "Actions/ActPlayMusic.h"
#include "Actions/ActEtherComm.h"
#include "Actions/ActSendComm.h"
#include "Actions/ActStartDialog.h"
#include "Actions/ActSetNNObj.h"
#include "Actions/ActNNPath.h"
#include "Actions/ActAdjAcct.h"
#include "Actions/ActAdjRep.h"
#include "Actions/ActAddCargo.h"
#include "Actions/ActRemoveCargo.h"
#include "Actions/ActGiveObjList.h"
#include "Actions/ActSetVibe.h"
#include "Actions/ActInvulnerable.h"
#include "Actions/ActLeaveGroup.h"
#include "Actions/ActLeaveMsn.h"
#include "Actions/ActSetLifeTime.h"
#include "Actions/ActMark.h"
#include "Actions/ActCloak.h"
#include "Actions/ActSetDockState.h"
#include "Actions/ActDockInstant.h"
#include "Actions/ActDisplayMsg.h"
#include "Actions/ActPlayNN.h"
#include "Actions/ActPopUpDialog.h"
#include "MissionBoard.h"
#include "ClientObjectives.h"

namespace Missions
{	
	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void LoadSettings()
	{
		char currentDirectory[MAX_PATH];
		GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
		const std::string missionDirectory = std::string(currentDirectory) + "\\flhook_plugins\\missions\\";
		if (!std::filesystem::is_directory(missionDirectory))
			return;

		// Read all mission files
		uint nextMissionId = 0;

		const std::regex filePattern(".+\\.ini", std::regex_constants::ECMAScript | std::regex_constants::icase);
		for (const auto& entry : std::filesystem::recursive_directory_iterator(missionDirectory))
		{
			const std::string fileName = wstos(entry.path().filename());
			INI_Reader ini;
			if (std::regex_match(fileName, filePattern) && ini.open((missionDirectory + fileName).c_str(), false))
			{
				uint missionId = 0;
				while (ini.read_header())
				{
					if (TryReadMissionHeadFromIni(nextMissionId, ini))
					{
						missionId = nextMissionId;
						nextMissionId++;
					}
					if (missions.empty())
						continue;
					Mission& mission = missions.at(missionId);
					TryReadMsnFormationFromIni(mission, ini);
					TryReadNpcFromIni(mission, ini);
					TryReadMsnNpcFromIni(mission, ini);
					TryReadMsnSolarFromIni(mission, ini);
					TryReadDialogFromIni(mission, ini);
					TryReadObjectiveListFromIni(mission, ini);

					if (ini.is_header("Trigger"))
					{
						uint id = 0;
						bool initiallyActive = false;
						Trigger::TriggerRepeatable repeatable = Trigger::TriggerRepeatable::Off;
						ConditionPtr condition = nullptr;
						std::vector<ActionPtr> actions;

						const auto start = ini.tell();
						while (ini.read_value())
						{
							if (ini.is_value("nickname"))
							{
								id = CreateIdOrNull(ini.get_value_string(0));
								break;
							}
						}
						ini.seek(start);

						const ConditionParent conditionParent(missionId, id);

						while (id && ini.read_value())
						{
							if (ini.is_value("InitState"))
							{
								initiallyActive = ToLower(ini.get_value_string(0)) == "active";
							}
							else if (ini.is_value("repeatable"))
							{
								const std::string& value = ToLower(ini.get_value_string(0));
								if (value == "auto")
									repeatable = Trigger::TriggerRepeatable::Auto;
								else if (value == "manual")
									repeatable = Trigger::TriggerRepeatable::Manual;
								else
									repeatable = Trigger::TriggerRepeatable::Off;
							}
							else if (ini.is_value("Act_DebugMsg"))
							{
								ActDebugMsgPtr action(new ActDebugMsg());
								action->message = ini.get_value_string(0);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_TerminateMsn"))
							{
								ActTerminateMsnPtr action(new ActTerminateMsn());
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SetMsnResult"))
							{
								ActSetMsnResultPtr action(new ActSetMsnResult());
								action->result = ToLower(ini.get_value_string(0)) == "success" ? Mission::MissionResult::Success : Mission::MissionResult::Failure;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_LeaveGroup"))
							{
								ActLeaveGroupPtr action(new ActLeaveGroup());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_LeaveMsn"))
							{
								ActLeaveMsnPtr action(new ActLeaveMsn());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								const std::string& value = ToLower(ini.get_value_string(1));
								if (value == "success")
									action->leaveType = LeaveMsnType::Success;
								else if (value == "failure")
									action->leaveType = LeaveMsnType::Failure;
								else
									action->leaveType = LeaveMsnType::Silent;
								action->failureStringId = ini.get_value_int(2);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_ActTrig"))
							{
								ActActTrigPtr action(new ActActTrig());
								for (size_t arg = 0, length = ini.get_num_parameters(); arg < length; arg++)
								{
									if (arg % 2 == 0)
									{
										ActActTrigEntry entry;
										entry.triggerId = CreateIdOrNull(ini.get_value_string(arg));
										action->triggers.push_back(entry);
									}
									else
										action->triggers.back().probability = ini.get_value_float(arg);
								}
								action->activate = true;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_DeactTrig"))
							{
								ActActTrigPtr action(new ActActTrig());
								for (size_t arg = 0, length = ini.get_num_parameters(); arg < length; arg++)
								{
									if (arg % 2 == 0)
									{
										ActActTrigEntry entry;
										entry.triggerId = CreateIdOrNull(ini.get_value_string(arg));
										action->triggers.push_back(entry);
									}
									else
										action->triggers.back().probability = ini.get_value_float(arg);
								}
								action->activate = false;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_ActMsn"))
							{
								ActActMsnPtr action(new ActActMsn());
								action->missionId = CreateIdOrNull(ini.get_value_string(0));
								for (int index = 1, length = ini.get_num_parameters(); index < length; index++)
								{
									const auto& value = ToLower(ini.get_value_string(index));
									if (value == "all")
									{
										action->playerLabelsToTransfer.clear();
										action->playerLabelsToTransfer.insert(ActActMsnAllPlayerLabels);
										break;
									}
									action->playerLabelsToTransfer.insert(CreateIdOrNull(value.c_str()));
								}
								actions.push_back(action);
							}
							else if (ini.is_value("Act_ActMsnTrig"))
							{
								ActActMsnTrigPtr action(new ActActMsnTrig());
								action->missionId = CreateIdOrNull(ini.get_value_string(0));
								for (size_t arg = 1, length = ini.get_num_parameters(); arg < length; arg++)
								{
									if (arg % 2 == 1)
									{
										ActActTrigEntry entry;
										entry.triggerId = CreateIdOrNull(ini.get_value_string(arg));
										action->triggers.push_back(entry);
									}
									else
										action->triggers.back().probability = ini.get_value_float(arg);
								}
								action->activate = true;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_DeactMsnTrig"))
							{
								ActActMsnTrigPtr action(new ActActMsnTrig());
								action->missionId = CreateIdOrNull(ini.get_value_string(0));
								for (size_t arg = 1, length = ini.get_num_parameters(); arg < length; arg++)
								{
									if (arg % 2 == 1)
									{
										ActActTrigEntry entry;
										entry.triggerId = CreateIdOrNull(ini.get_value_string(arg));
										action->triggers.push_back(entry);
									}
									else
										action->triggers.back().probability = ini.get_value_float(arg);
								}
								action->activate = false;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AddLabel"))
							{
								ActAddLabelPtr action(new ActAddLabel());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->label = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_RemoveLabel"))
							{
								ActRemoveLabelPtr action(new ActRemoveLabel());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->label = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_LightFuse"))
							{
								ActLightFusePtr action(new ActLightFuse());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->fuse = CreateIdOrNull(ini.get_value_string(1));
								action->timeOffset = ini.get_value_float(2);
								action->lifetimeOverride = ini.get_value_float(3);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_UnlightFuse"))
							{
								ActUnlightFusePtr action(new ActUnlightFuse());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->fuse = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
								}
							else if (ini.is_value("Act_SetLifeTime"))
							{
								ActSetLifeTimePtr action(new ActSetLifeTime());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->lifeTime = ini.get_value_float(1);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Mark"))
							{
								ActMarkPtr action(new ActMark());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->targetObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								if (ini.get_num_parameters() > 2)
									action->marked = ini.get_value_bool(2);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Cloak"))
							{
								ActCloakPtr action(new ActCloak());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								if (ini.get_num_parameters() > 1)
									action->cloaked = ini.get_value_bool(1);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SpawnSolar"))
							{
								ActSpawnSolarPtr action(new ActSpawnSolar());
								action->solarName = ToLower(ini.get_value_string(0));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SpawnShip"))
							{
								ActSpawnShipPtr action(new ActSpawnShip());
								action->msnNpcId = CreateIdOrNull(ini.get_value_string(0));
								const auto objectivesName = ToLower(ini.get_value_string(1));
								action->objectivesId = objectivesName == "no_ol" ? 0 : CreateID(objectivesName.c_str());
								if (ini.get_num_parameters() > 2)
								{
									action->position.x = ini.get_value_float(2);
									action->position.y = ini.get_value_float(3);
									action->position.z = ini.get_value_float(4);
								}
								if (ini.get_num_parameters() > 5)
								{
									Vector orientation;
									orientation.x = ini.get_value_float(5);
									orientation.y = ini.get_value_float(6);
									orientation.z = ini.get_value_float(7);
									action->orientation = EulerMatrix(orientation);
								}
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SpawnFormation"))
							{
								ActSpawnFormationPtr action(new ActSpawnFormation());
								action->msnFormationId = CreateIdOrNull(ini.get_value_string(0));
								const auto objectivesName = ToLower(ini.get_value_string(1));
								action->objectivesId = objectivesName == "no_ol" ? 0 : CreateID(objectivesName.c_str());
								if (ini.get_num_parameters() > 2)
								{
									action->position.x = ini.get_value_float(2);
									action->position.y = ini.get_value_float(3);
									action->position.z = ini.get_value_float(4);
								}
								if (ini.get_num_parameters() > 5)
								{
									action->rotation.x = ini.get_value_float(5);
									action->rotation.y = ini.get_value_float(6);
									action->rotation.z = ini.get_value_float(7);
								}
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Destroy"))
							{
								ActDestroyPtr action(new ActDestroy());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->destroyType = ToLower(ini.get_value_string(1)) == "explode" ? DestroyType::EXPLODE : DestroyType::VANISH;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_PlaySoundEffect"))
							{
								ActPlaySoundEffectPtr action(new ActPlaySoundEffect());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->soundId = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_PlayMusic"))
							{
								ActPlayMusicPtr action(new ActPlayMusic());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->music.spaceMusic = ToLower(ini.get_value_string(1)) != "default" ? CreateIdOrNull(ini.get_value_string(1)) : 0;
								action->music.dangerMusic = ToLower(ini.get_value_string(2)) != "default" ? CreateIdOrNull(ini.get_value_string(2)) : 0;
								action->music.battleMusic = ToLower(ini.get_value_string(3)) != "default" ? CreateIdOrNull(ini.get_value_string(3)) : 0;
								action->music.overrideMusic = ToLower(ini.get_value_string(4)) != "none" ? CreateIdOrNull(ini.get_value_string(4)) : 0;
								action->music.crossFadeDurationInS = ini.get_value_float(5);
								if (ini.get_num_parameters() > 6)
									action->music.playOnce = ini.get_value_bool(6);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Ethercomm"))
							{
								ActEtherCommPtr action(new ActEtherComm());
								action->id = CreateIdOrNull(ini.get_value_string(0));
								action->receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								action->senderVoiceId = CreateIdOrNull(ini.get_value_string(2));
								uint pos = 3;
								while (!ini.is_value_empty(pos))
								{
									const char* val = ini.get_value_string(pos);
									// Make sure we do not go beyond the following numeric value.
									char* end;
									strtol(val, &end, 10);
									if (end != val)
										break;
									action->lines.push_back(CreateIdOrNull(val));
									pos++;
								}
								if (!ini.is_value_empty(pos++))
									action->delay = ini.get_value_float(pos - 1);
								if (ini.get_num_parameters() > pos)
									action->global = ini.get_value_bool(pos++);
								action->senderIdsName = ini.get_value_int(pos++);
								action->costume.head = CreateIdOrNull(ini.get_value_string(pos++));
								action->costume.body = CreateIdOrNull(ini.get_value_string(pos++));
								int count = 0;
								while (count < 8)
								{
									const auto val = ini.get_value_string(pos++);
									if (strlen(val) == 0)
										break;
									action->costume.accessory[count++] = CreateIdOrNull(val);
								}
								action->costume.accessories = count;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SendComm"))
							{
								ActSendCommPtr action(new ActSendComm());
								action->id = CreateIdOrNull(ini.get_value_string(0));
								action->receiverObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								action->senderObjName = CreateIdOrNull(ini.get_value_string(2));
								uint pos = 3;
								while (!ini.is_value_empty(pos))
								{
									const char* val = ini.get_value_string(pos);
									// Make sure we do not go beyond the following numeric value.
									char* end;
									strtol(val, &end, 10);
									if (end != val)
										break;
									action->lines.push_back(CreateIdOrNull(val));
									pos++;
								}
								if (!ini.is_value_empty(pos++))
									action->delay = ini.get_value_float(pos - 1);
								if (ini.get_num_parameters() > pos)
									action->global = ini.get_value_bool(pos);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_StartDialog"))
							{
								ActStartDialogPtr action(new ActStartDialog());
								action->dialogId = CreateIdOrNull(ini.get_value_string(0));
								actions.push_back(action);
								}
							else if (ini.is_value("Act_SetNNObj"))
							{
								ActSetNNObjPtr action(new ActSetNNObj());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->message = FmtStr(ini.get_value_int(1), 0);
								action->systemId = CreateIdOrNull(ini.get_value_string(2));
								action->position.x = ini.get_value_float(3);
								action->position.y = ini.get_value_float(4);
								action->position.z = ini.get_value_float(5);
								if (ini.get_num_parameters() > 6)
									action->bestRoute = ini.get_value_bool(6);
								action->targetObjName = CreateIdOrNull(ini.get_value_string(7));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_NNPath"))
							{
								ActNNPathPtr action(new ActNNPath());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->message = ini.get_value_int(1);
								action->systemId = CreateIdOrNull(ini.get_value_string(2));
								action->position.x = ini.get_value_float(3);
								action->position.y = ini.get_value_float(4);
								action->position.z = ini.get_value_float(5);
								if (ini.get_num_parameters() > 6)
									action->bestRoute = ini.get_value_bool(6);
								action->targetObjName = CreateIdOrNull(ini.get_value_string(7));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AdjAcct"))
							{
								ActAdjAcctPtr action(new ActAdjAcct());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->cash = ini.get_value_int(1);
								if (ini.get_num_parameters() > 2)
									action->splitBetweenPlayers = ini.get_value_bool(2);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AdjRep"))
							{
								ActAdjRepPtr action(new ActAdjRep());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								pub::Reputation::GetReputationGroup(action->groupId, ini.get_value_string(1));
								const auto& val = ToLower(ini.get_value_string(2));
								if (val == "objectdestruction")
									action->reason = Empathies::ReputationChangeReason::ObjectDestruction;
								else if (val == "missionsuccess")
										action->reason = Empathies::ReputationChangeReason::MissionSuccess;
								else if (val == "missionfailure")
									action->reason = Empathies::ReputationChangeReason::MissionFailure;
								else if (val == "missionabortion")
									action->reason = Empathies::ReputationChangeReason::MissionAbortion;
								else
									action->change = ini.get_value_float(2);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_AddCargo"))
							{
								ActAddCargoPtr action(new ActAddCargo());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->itemId = CreateIdOrNull(ini.get_value_string(1));
								action->count = std::max<uint>(0, ini.get_value_int(2));
								if (ini.get_num_parameters() > 3)
									action->missionFlagged = ini.get_value_bool(3);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_RemoveCargo"))
							{
								ActRemoveCargoPtr action(new ActRemoveCargo());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->itemId = CreateIdOrNull(ini.get_value_string(1));
								action->count = std::max<uint>(0, ini.get_value_int(2));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_GiveObjList"))
							{
								ActGiveObjListPtr action(new ActGiveObjList());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->objectivesId = CreateIdOrNull(ini.get_value_string(1));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SetVibe"))
							{
								ActSetVibePtr action(new ActSetVibe());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->targetObjNameOrLabel = CreateIdOrNull(ini.get_value_string(1));
								action->reputation = ini.get_value_float(2);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Invulnerable"))
							{
								ActInvulnerablePtr action(new ActInvulnerable());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								if (ini.get_num_parameters() > 1)
									action->preventNonPlayerDamage = ini.get_value_bool(1);
								if (ini.get_num_parameters() > 2)
									action->preventPlayerDamage = ini.get_value_bool(2);
								if (ini.get_num_parameters() > 3)
									action->maxHpLossPercentage = ini.get_value_float(3);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_Relocate"))
							{
								ActRelocatePtr action(new ActRelocate());
								action->objName = CreateIdOrNull(ini.get_value_string(0));
								action->systemId = CreateIdOrNull(ini.get_value_string(1));
								action->position.x = ini.get_value_float(2);
								action->position.y = ini.get_value_float(3);
								action->position.z = ini.get_value_float(4);
								if (ini.get_num_parameters() > 7)
								{
									Vector rotation;
									rotation.x = ini.get_value_float(5);
									rotation.y = ini.get_value_float(6);
									rotation.z = ini.get_value_float(7);
									action->orientation = EulerMatrix(rotation);
								}
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SetDockState"))
							{
								ActSetDockStatePtr action(new ActSetDockState());
								action->objNameOrLabel = CreateIdOrNull(ini.get_value_string(0));
								action->dockHardpoint = ToLower(ini.get_value_string(1));
								action->opened = ToLower(ini.get_value_string(2)) == "opened";
								actions.push_back(action);
							}
							else if (ini.is_value("Act_DockInstant"))
							{
								ActDockInstantPtr action(new ActDockInstant());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->targetObjName = CreateIdOrNull(ini.get_value_string(1));
								action->dockHardpoint = ToLower(ini.get_value_string(2));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_DisplayMsg"))
							{
								ActDisplayMsgPtr action(new ActDisplayMsg());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->stringId = ini.get_value_int(1);
								actions.push_back(action);
							}
							else if (ini.is_value("Act_PlayNN"))
							{
								ActPlayNNPtr action(new ActPlayNN());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								for (size_t index = 1, length = ini.get_num_parameters(); index < length; index++)
									action->soundIds.push_back(CreateIdOrNull(ini.get_value_string(index)));
								actions.push_back(action);
							}
							else if (ini.is_value("Act_PopUpDialog"))
							{
								ActPopUpDialogPtr action(new ActPopUpDialog());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->headingId = ini.get_value_int(1);
								action->messageId = ini.get_value_int(2);
								action->buttons = 0;
								for (size_t index = 3, length = ini.get_num_parameters(); index < length; index++)
								{
									const auto& button = ToLower(ini.get_value_string(index));
									if (button == "close")
										action->buttons = action->buttons | PopupDialogButton::CENTER_OK;
									else if (button == "yes")
										action->buttons = action->buttons | PopupDialogButton::LEFT_YES;
									else if (button == "no")
										action->buttons = action->buttons | PopupDialogButton::CENTER_NO;
									else if (button == "later")
										action->buttons = action->buttons | PopupDialogButton::RIGHT_LATER;
								}
								if (!action->buttons)
									action->buttons = PopupDialogButton::CENTER_OK;
								actions.push_back(action);
							}
							else if (ini.is_value("Act_SetShipAndLoadout"))
							{
								ActSetShipAndLoadoutPtr action(new ActSetShipAndLoadout());
								action->label = CreateIdOrNull(ini.get_value_string(0));
								action->shipArchetypeId = CreateIdOrNull(ini.get_value_string(1));
								action->loadoutId = CreateIdOrNull(ini.get_value_string(2));
								actions.push_back(action);
							}
							else
							{
								const auto& cnd = TryReadConditionFromIni(conditionParent, ini);
								if (cnd != nullptr)
								{
									if (condition != nullptr)
										ConPrint(L"Trigger " + std::to_wstring(id) + L" already has a condition! Overriding.");
									condition = ConditionPtr(cnd);
								}
							}
						}

						if (id)
						{
							missions.at(missionId).triggers.try_emplace(id, id, missionId, initiallyActive, repeatable);
							Trigger& trigger = missions.at(missionId).triggers.at(id);
							if (condition == nullptr)
								ConPrint(L"Trigger " + std::to_wstring(id) + L" has no condition! Falling back to Cnd_True\n");
							trigger.condition = condition != nullptr ? condition : ConditionPtr(new CndTrue({ missionId, id }));
							trigger.actions = actions;
						}
					}
				}
				ini.close();
			}
		}
	}
	
	void StartMissionByOfferId(const uint offerId, const uint startingClientId, const std::vector<uint>& clientIds)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.offerId == offerId && missionEntry.second.CanBeStarted())
			{
				auto& mission = missionEntry.second;
				mission.AddLabelToObject(MissionObject(MissionObjectType::Client, startingClientId), CreateID("initial_player"));
				const uint labelId = CreateID("players");
				for (const auto clientId : clientIds)
					mission.AddLabelToObject(MissionObject(MissionObjectType::Client, clientId), labelId);
				mission.Start();
				return;
			}
		}
	}

	bool IsPartOfOfferedJob(const uint clientId)
	{
		for (const auto& mission : missions)
		{
			if (mission.second.offerId != 0 && mission.second.clientIds.contains(clientId))
				return true;
		}
		return false;
	}

	void RemoveClientFromCurrentOfferedJob(const uint clientId)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.offerId != 0 && missionEntry.second.clientIds.contains(clientId))
			{
				missionEntry.second.RemoveClient(clientId);
				return;
			}
		}
	}

	bool initialized = false;
	void Initialize()
	{
		if (initialized)
			return;
		initialized = true;

		ConPrint(L"Initializing Missions...\n");

		LoadSettings();

		ConPrint(L"Starting initial missions...\n");

		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.initiallyActive)
				missionEntry.second.Start();
		}

		ConPrint(L"Done initializing Missions\n");
	}

	static void ClearMissions()
	{
		auto it = missions.begin();
		while (it != missions.end())
		{
			MissionBoard::DeleteOffer(it->second.offerId);
			it = missions.erase(it);
		}
	}

	void __stdcall Shutdown()
	{
		ClearMissions();
		returncode = DEFAULT_RETURNCODE;
	}

	static void RemoveObjectFromMissions(const uint objId)
	{
		// Removing an object from a mission could potentially end and remove it. So first gather the mission IDs and then delete them one by one.
		std::vector<uint> ids;
		for (const auto& entry : missions)
			ids.push_back(entry.first);

		for (const auto id : ids)
		{
			if (const auto& entry = missions.find(id); entry != missions.end())
				entry->second.RemoveObject(objId);
		}
	}

	static void RemoveClientFromMissions(const uint clientId)
	{
		ClientObjectives::ClearClientObjectives(clientId, 0);
		// Removing a client from a mission could potentially end and remove it. So first gather the mission IDs and then delete them one by one.
		std::vector<uint> ids;
		for (const auto& entry : missions)
			ids.push_back(entry.first);

		for (const auto id : ids)
		{
			if (const auto& entry = missions.find(id); entry != missions.end())
				entry->second.RemoveClient(clientId);
		}

		// Remove all mission cargo to avoid it being left-over in the cargo hold.
		for (const auto& equip : Players[clientId].equipDescList.equip)
		{
			if (equip.bMission)
				pub::Player::RemoveCargo(clientId, equip.sID, equip.iCount);
		}
	}

	void __stdcall ObjDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
	{
		RemoveObjectFromMissions(killedObject->cobj->id);
		returncode = DEFAULT_RETURNCODE;
	}

	std::unordered_map<uint, CHARACTER_ID> lastCharacterByClientId;

	void __stdcall CharacterSelect(const CHARACTER_ID& cId, unsigned int clientId)
	{
		const auto& foundEntry = lastCharacterByClientId.find(clientId);
		if (foundEntry == lastCharacterByClientId.end() || !(foundEntry->second == cId))
			RemoveClientFromMissions(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall CharacterSelect_AFTER(const CHARACTER_ID& cId, unsigned int clientId)
	{
		lastCharacterByClientId[clientId] = cId;
		returncode = DEFAULT_RETURNCODE;
	}

	void __stdcall DisConnect(unsigned int clientId, enum EFLConnection p2)
	{
		lastCharacterByClientId.erase(clientId);
		RemoveClientFromMissions(clientId);
		returncode = DEFAULT_RETURNCODE;
	}

	static bool StartMission(const std::string& missionName)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.name == missionName && !missionEntry.second.IsActive())
			{
				missionEntry.second.Start();	
				return true;
			}
		}
		return false;
	}

	static bool EndMission(const std::string& missionName)
	{
		for (auto& missionEntry : missions)
		{
			if (missionEntry.second.name == missionName)
			{
				missionEntry.second.End();
				return true;
			}
		}
		return false;
	}

	#define IS_CMD(a) !wscCmd.compare(L##a)

	bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd)
	{
		if (IS_CMD("start_mission"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (StartMission(targetNickname))
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" started.");
			else
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" not found or already running.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		else if (IS_CMD("stop_mission"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
			if (EndMission(targetNickname))
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" ended.");
			else
				PrintUserCmdText(clientId, L"Mission " + stows(targetNickname) + L" not found or already stopped.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}
		else if (IS_CMD("reload_missions"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			ClearMissions();
			initialized = false;
			PrintUserCmdText(clientId, L"Stopped and reloaded all missions.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		else if (IS_CMD("act_trigger"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string msnNickname = wstos(ToLower(cmds->ArgStr(1)));
			const std::string trigNickname = wstos(ToLower(cmds->ArgStr(2)));
			ActActMsnTrig action;
			action.missionId = CreateIdOrNull(msnNickname.c_str());
			ActActTrigEntry entry;
			entry.triggerId = CreateIdOrNull(trigNickname.c_str());
			action.triggers.push_back(entry);
			action.activate = true;
			Mission msn("", 0, false, false);
			action.Execute(msn, MissionObject(MissionObjectType::Client, clientId));

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		else if (IS_CMD("deact_trigger"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_EVENTMODE))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			const std::string msnNickname = wstos(ToLower(cmds->ArgStr(1)));
			const std::string trigNickname = wstos(ToLower(cmds->ArgStr(2)));
			ActActMsnTrig action;
			action.missionId = CreateIdOrNull(msnNickname.c_str());
			ActActTrigEntry entry;
			entry.triggerId = CreateIdOrNull(trigNickname.c_str());
			action.triggers.push_back(entry);
			action.activate = true;
			Mission msn("", 0, false, false);
			action.Execute(msn, MissionObject(MissionObjectType::Client, clientId));

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		else if (IS_CMD("getpos"))
		{
			const uint clientId = ((CInGame*)cmds)->iClientID;
			if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_SUPERADMIN))
			{
				PrintUserCmdText(clientId, L"ERR No permission");
				returncode = SKIPPLUGINS_NOFUNCTIONCALL;
				return false;
			}

			uint shipId;
			pub::Player::GetShip(clientId, shipId);
			IObjRW* inspect;
			StarSystem* system;
			if (shipId && GetShipInspect(shipId, inspect, system))
				PrintUserCmdText(clientId, L"Pos: " + std::to_wstring(inspect->cobj->vPos.x) + L", " + std::to_wstring(inspect->cobj->vPos.y) + L", " + std::to_wstring(inspect->cobj->vPos.z));
			else
				PrintUserCmdText(clientId, L"You must be in space for that.");

			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
			return true;
		}

		returncode = DEFAULT_RETURNCODE;
		return false;
	}
}