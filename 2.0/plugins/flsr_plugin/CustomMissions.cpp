#include "main.h"

namespace CustomMissions {

	std::list<CustomMission> lCustomMission;

	void LoadMissions() {

		char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string sMissionStoreFolder = std::string(szCurDir) + MISSION_STORE;

        // Load all missions from the mission store
		lCustomMission.clear();
        for (const auto &p : std::filesystem::recursive_directory_iterator(sMissionStoreFolder)) {
            if (!std::filesystem::is_directory(p)) {
                std::filesystem::path f(p.path());
                if (Tools::endsWith(f.filename().string(), ".ini"))
                {
                    CustomMission NewMission;
					
					//General
                    NewMission.iMissionID = IniGetI(p.path().string(), "General", "MissionID", lCustomMission.size());
                    NewMission.scMissionFilepath = p.path().string();
                    NewMission.scMissionName = IniGetS(p.path().string(), "General", "MissionName", "");
                    NewMission.scMissionDesc = IniGetS(p.path().string(), "General", "MissionDesc", "");
                    NewMission.scMissionType = IniGetS(p.path().string(), "General", "MissionType", "");
                    NewMission.bSPMission = IniGetB(p.path().string(), "General", "SPMission", false);
                    NewMission.bMPMission = IniGetB(p.path().string(), "General", "MPMission", false);
					
					//Reward
                    NewMission.bRewardCredits = IniGetB(p.path().string(), "Reward", "Credits", false);
                    NewMission.iRewardCredits = IniGetI(p.path().string(), "Reward", "CreditsAmount", 0);
					NewMission.bRewardReputation = IniGetB(p.path().string(), "Reward", "Reputation", false);
					NewMission.iRewardReputation = IniGetI(p.path().string(), "Reward", "ReputationAmount", 0);
                    NewMission.scRewardReputationNickname = IniGetS(p.path().string(), "Reward", "ReputationNickname", "");
                    NewMission.bRewardShip = IniGetB(p.path().string(), "Reward", "Ship", false);
                    NewMission.scRewardShipNickname = IniGetS(p.path().string(), "Reward", "ShipNickname", "");
                    NewMission.bRewardEquip = IniGetB(p.path().string(), "Reward", "Equip", false);
                    NewMission.scRewardEquipNickname = IniGetS(p.path().string(), "Reward", "EquipNickname", "");
                    NewMission.iRewardEquip = IniGetI(p.path().string(), "Reward", "EquipAmount", 0);
					
					//Trade Mission
                    NewMission.scTargetBaseNickname = IniGetS(p.path().string(), "TradeMission", "TargetBaseNickname", "");
                    NewMission.scGoodToTradeNickname = IniGetS(p.path().string(), "TradeMission", "GoodToTradeNickname", "");
                    NewMission.iAmountToTrade = IniGetI(p.path().string(), "TradeMission", "AmountToTrade", 0);

					//Kill NPC Mission
                    NewMission.scNPCType = IniGetS(p.path().string(), "KillNPCMission", "NPCType", "");
                    NewMission.iAmountofNPCs = IniGetI(p.path().string(), "KillNPCMission", "NPCAmount", 0);
                    NewMission.iAmountofWaves = IniGetI(p.path().string(), "KillNPCMission", "WaveAmount", 0);
					NewMission.bRewardKill = IniGetB(p.path().string(), "KillNPCMission", "RewardKill", false);
					NewMission.bRewardGroup = IniGetB(p.path().string(), "KillNPCMission", "RewardGroup", false);
					NewMission.bKillNamedNPC = IniGetB(p.path().string(), "KillNPCMission", "KillNamedNPC", false);
					NewMission.scNamedNPCName = IniGetS(p.path().string(), "KillNPCMission", "NamedNPCName", "");
					
					//Player Hunt Mission Data is calculated
					
					//Mining Mission
					NewMission.scGoodToMineNickname = IniGetS(p.path().string(), "MiningMission", "GoodToMineNickname", "");
					NewMission.iAmountToMine = IniGetI(p.path().string(), "MiningMission", "AmountToMine", 0);
					
					//Load WayPoints
					NewMission.bSendMissionWaypoints = IniGetB(p.path().string(), "Waypoints", "SendMissionWaypoints", false);
					NewMission.bSendMissionWaypointsToGroup = IniGetB(p.path().string(), "Waypoints", "SendMissionWaypointsToGroup", false);
					
                    //Load all Waypoints
                    NewMission.lPlayerWaypoints.clear();
                    for (int i = 0;; i++) {
						PlayerWaypoint Waypoint;
                        char szBuf[64];
                        sprintf(szBuf, "Waypoint%u", i);
						
                        // Coords
						Waypoint.X = IniGetS(p.path().string(), szBuf, "X", "");
                        Waypoint.Y = IniGetS(p.path().string(), szBuf, "Y", "");
                        Waypoint.Z = IniGetS(p.path().string(), szBuf, "Z", "");
						
						//System
						Waypoint.iSystemID = IniGetI(p.path().string(), szBuf, "System", 0);

                        //SolarObject
						Waypoint.iSolarObjectID = IniGetI(p.path().string(), szBuf, "SolarObject", 0);
						
                        if (Waypoint.iSystemID == 0)
                            break;

						//Push
                        NewMission.lPlayerWaypoints.push_back(Waypoint);
                    }
					
					//MissionStart POPUP
					NewMission.bMissionStartPopup = IniGetB(p.path().string(), "Popups", "MissionStartPopup", false);
					NewMission.iMissionStartPopupHead = IniGetI(p.path().string(), "Popups", "MissionStartPopupHead", 0);
					NewMission.iMissionStartPopupBody = IniGetI(p.path().string(), "Popups", "MissionStartPopupBody", 0);
					
					//MissionEnd POPUP
					NewMission.bMissionEndPopup = IniGetB(p.path().string(), "Popups", "MissionEndPopup", false);
					NewMission.iMissionEndPopupHead = IniGetI(p.path().string(), "Popups", "MissionEndPopupHead", 0);
					NewMission.iMissionEndPopupBody = IniGetI(p.path().string(), "Popups", "MissionEndPopupBody", 0);
					
					//MissionStart Text
					NewMission.bMissionStartText = IniGetB(p.path().string(), "MissionText", "MissionStartText", false);
					NewMission.iMissionStartText = IniGetI(p.path().string(), "MissionText", "MissionStartTextID", 0);
					
					//MissionEnd Text
					NewMission.bMissionEndText = IniGetB(p.path().string(), "MissionText", "MissionEndText", false);
					NewMission.iMissionEndText = IniGetI(p.path().string(), "MissionText", "MissionEndTextID", 0);

					//Push
					lCustomMission.push_back(NewMission);
                }
            }
        }

		
	}

	
	void Send_WPs(uint iClientID, std::list <CustomMissions::PlayerWaypoint> lWP, bool bBestPath)
	{	
		int iCountWaypoints = lWP.size();
		std::string DataToSend;
		if (bBestPath)
		{
			std::string x;
			std::string y;
			std::string z;
			std::string sysID;
			std::string objectID;
			//Send BestPath
			//Example std::string str2 = "WPBP[x={-33356,2444},y={0,0},z={25834,55441},sysID={2818046082,2818046082},objectID={0,0},numberofWP=2]";
			int iIter = 0;
			for (std::list<PlayerWaypoint>::iterator i = lWP.begin(); i != lWP.end(); ++i)
			{
				if (iIter != iCountWaypoints)
				{
					x += i->X + ",";
					y += i->Y + ",";
					z += i->Z + ",";
					sysID += std::to_string(i->iSystemID) + ",";
					objectID += std::to_string(i->iSolarObjectID) + ",";
				}
				else
				{
					x += i->X;
					y += i->Y;
					z += i->Z;
					sysID += std::to_string(i->iSystemID);
					objectID += std::to_string(i->iSolarObjectID);
				
				}
				iIter ++ ;
			}
			
			DataToSend = "WPBP[x={" + x + "},y={" + y + "},z={" + z + "},sysID={" + sysID + "},objectID={" + objectID + "},numberofWP=" + std::to_string(iCountWaypoints) + "]";

			ClientController::Send_ControlMsg(true, iClientID, stows(DataToSend)); // This dont work
			PrintUserCmdText(iClientID, L"Sending WP: %s", stows(DataToSend));// This work
		}
		else
		{
			//Send Single WP in Loop
			//Example std::string str = "WP{x=-33356,y=0,z=-25834,sysID=2208796239}";
			for (std::list<PlayerWaypoint>::iterator i = lWP.begin(); i != lWP.end(); ++i)
			{
				DataToSend = "WP{x=";
				DataToSend += (*i).X;
				DataToSend += ",y=";
				DataToSend += (*i).Y;
				DataToSend += ",z=";
				DataToSend += (*i).Z;
				DataToSend += ",sysID=";
				DataToSend += std::to_string((*i).iSystemID);
				DataToSend += "}";

				ClientController::Send_ControlMsg(true, iClientID, stows(DataToSend)); // This dont work
				PrintUserCmdText(iClientID, L"Sending WP: %s", stows(DataToSend));// This work
			}
		}
	}

} // namespace CustomMissions