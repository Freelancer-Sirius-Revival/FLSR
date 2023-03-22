#include "Main.h"

namespace Cloak {

	//WarmUpCloak mPlayerWarmUpCloak[MAX_CLIENT_ID + 1];
	//PlayerCloakInfo PlayerCloakData[MAX_CLIENT_ID + 1];
	std::map<std::wstring, WarmUpCloak> mPlayerWarmUpCloak;
	std::map<std::wstring, PlayerCloakInfo> mPlayerCloakData;
	std::list<CloakDeviceInfo> lCloakDeviceList;

	void LoadCloakSettings()
	{
		// Konfigpfad
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		std::string scPluginCfgFile = std::string(szCurDir) + CLOAK_CONFIG_FILE;

		//Clear old data
		lCloakDeviceList.clear();
		
		//Lade Cloaking Devices to List		
		for (int i = 0;; i++) {
			char szBuf[64];
			sprintf(szBuf, "CloakDevice%u", i);

			//Read CloakDevice
			std::string scCloakDeviceNickname = IniGetS(scPluginCfgFile, szBuf, "Nickname", "");
			float fCloakCapacity = IniGetF(scPluginCfgFile, szBuf, "Capacity", 0.0f);
			float fPowerUsageToRecharge = IniGetF(scPluginCfgFile, szBuf, "PowerUsageToRecharge", 0.0f);
			float fCloakPowerUsageWhileCloaked = IniGetF(scPluginCfgFile, szBuf, "PowerUsageWhileCloaked", 0.0f);
			float fMinRequiredCapacityToCloak = IniGetF(scPluginCfgFile, szBuf, "MinRequiredCapacityToCloak ", 0.0f);
			bool bUseShipPowerToRecharge = IniGetB(scPluginCfgFile, szBuf, "UseShipPowerToRecharge", false);
			bool bShieldDownOnCloaking = IniGetB(scPluginCfgFile, szBuf, "ShieldDownOnCloaking", false);
			bool bShieldDownWhileCloaking = IniGetB(scPluginCfgFile, szBuf, "ShieldDownWhileCloaking", false);
			bool bCanUseCloakModule = IniGetB(scPluginCfgFile, szBuf, "CanUseCloakModule", false);
			float fCloakWarmUpDuration = IniGetF(scPluginCfgFile, szBuf, "CloakWarmUpDuration", 0.0f);
			float fCloakEffectDuration = IniGetF(scPluginCfgFile, szBuf, "CloakEffectDuration", 3.0f);
			float fUncloakEffectDuration = IniGetF(scPluginCfgFile, szBuf, "UncloakEffectDuration", 3.0f);

			

			if (scCloakDeviceNickname == "")
				break;
			
			//Create New CloakDevice
			CloakDeviceInfo CloakDevice;
			CloakDevice.scCloakDeviceNickname = scCloakDeviceNickname;
			CloakDevice.iCloakDeviceArchID = CreateID(scCloakDeviceNickname.c_str());
			CloakDevice.fCloakCapacity = fCloakCapacity;
			CloakDevice.fPowerUsageToRecharge = fPowerUsageToRecharge / 1000 * 250;
			CloakDevice.fCloakPowerUsageWhileCloaked = fCloakPowerUsageWhileCloaked / 1000 * 250;;
			CloakDevice.fMinRequiredCapacityToCloak = fMinRequiredCapacityToCloak;
			CloakDevice.bUseShipPowerToRecharge = bUseShipPowerToRecharge;
			CloakDevice.bShieldDownOnCloaking = bShieldDownOnCloaking;
			CloakDevice.bShieldDownWhileCloaking = bShieldDownWhileCloaking;
			CloakDevice.bCanUseCloakModule = bCanUseCloakModule;
			CloakDevice.iCloakWarmUpDuration = (int)(1000 * fCloakWarmUpDuration);
			CloakDevice.iCloakEffectDuration = (int)(1000 * fCloakEffectDuration);
			CloakDevice.iUncloakEffectDuration = (int)(1000 * fUncloakEffectDuration);
			
			//Add Device to List
			lCloakDeviceList.push_back(CloakDevice);
			
			
			if (scCloakDeviceNickname == "")
				break;


		}
	}

	void InstallCloak(uint iClientID)
	{
		HK_ERROR err;


		if (!HkIsValidClientID(iClientID)) {
			return;
		}
		if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
			return;
		std::wstring wscCharFileName;
		if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
			return;
		}
		
		mPlayerCloakData[wscCharFileName].bCanCloak = false;

		std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);

		std::list<CARGO_INFO> lstEquipment;
		int iRemaining;
		HkEnumCargo(wscCharname, lstEquipment, iRemaining);
		
		std::list<CARGO_INFO> lstMounted;
		for (auto& cargo : lstEquipment) {

			if (cargo.bMounted) {
				// check for mounted cloak device
				std::list<CloakDeviceInfo>::iterator CloakDevice = lCloakDeviceList.begin();
				while (CloakDevice != lCloakDeviceList.end()) {
					uint iArchIDCloak = CreateID(CloakDevice->scCloakDeviceNickname.c_str());
					if (cargo.iArchID == iArchIDCloak) {
						mPlayerCloakData[wscCharFileName].bInitialCloak = true;
						mPlayerCloakData[wscCharFileName].bCanCloak = true;
						mPlayerCloakData[wscCharFileName].bIsCloaking = false;
						mPlayerCloakData[wscCharFileName].bWantsCloak = false;
						mPlayerCloakData[wscCharFileName].bCloaked = false;
						mPlayerCloakData[wscCharFileName].iCloakSlot = cargo.iID;
						mPlayerCloakData[wscCharFileName].iCloakDeviceArchID = cargo.iArchID;
						mPlayerCloakData[wscCharFileName].tmCloakTime = 0;
											
						//Reset
						mPlayerCloakData[wscCharFileName].fCloakCap = mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity;

						//Send MaxEnergy to Client
						int iMaxEnergy = static_cast<int>(mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity);
						ClientController::Send_ControlMsg(false, iClientID, L"MaxEnergy(" + std::to_wstring(iMaxEnergy) + L")");
						
						//Send CloakCap to Client
						ClientController::Send_ControlMsg(false, iClientID, L"CloakEnergy(" + std::to_wstring(iMaxEnergy) + L")");

						//Send MinRequiredCapacityToCloak to Client
						int iMinRequiredCapacityToCloak = static_cast<int>(mPlayerCloakData[wscCharFileName].PlayerCloakData.fMinRequiredCapacityToCloak);
						ClientController::Send_ControlMsg(false, iClientID, L"NeededEnergy(" + std::to_wstring(iMinRequiredCapacityToCloak) + L")");
						
						//Disable UI
						ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");

						//Update Energy Data
						ClientController::Send_ControlMsg(false, iClientID, L"getpower");
					
						return;

					}
					CloakDevice++;
				}
			}
		}

	}

	//Uncloak at Spawn (every 2000ms Check)
	void CloakInstallTimer2000ms() {
		//CloakModule
		HK_ERROR err;


		if (Modules::GetModuleState("CloakModule"))
		{
			struct PlayerData* pd = 0;
			while (pd = Players.traverse_active(pd)) {
				uint iClientID = HkGetClientIdFromPD(pd);
				
				if (iClientID < 1 || iClientID > MAX_CLIENT_ID)
					continue;

				if (!HkIsValidClientID(iClientID)) {
					continue;
				}
				if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
					continue;
				std::wstring wscCharFileName;
				if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
					continue;
				}
				
				if (mPlayerCloakData[wscCharFileName].bInitialCloak) {
					ClientController::Send_ControlMsg(true, iClientID, L"_cloakoff");
					mPlayerCloakData[wscCharFileName].bInitialCloak = false;
					

					XActivateEquip ActivateEq;
					ActivateEq.bActivate = false;
					ActivateEq.iSpaceID = ClientInfo[iClientID].iShip;
					ActivateEq.sID = Cloak::mPlayerCloakData[wscCharFileName].iCloakSlot;
					Server.ActivateEquip(iClientID, ActivateEq);

				}

			}
		}
	}

	//Check for CloakWarmUp (every 1000ms)
	void WarmUpCloakTimer1000ms() {
		HK_ERROR err;


		//CloakModule
		if (Modules::GetModuleState("CloakModule"))
		{
			struct PlayerData* pd = 0;
			while (pd = Players.traverse_active(pd)) {
				uint iClientID = HkGetClientIdFromPD(pd);
				
				if (iClientID < 1 || iClientID > MAX_CLIENT_ID)
					continue;

				if (!HkIsValidClientID(iClientID)) {
					continue;
				}
				if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
					continue;
				std::wstring wscCharFileName;
				if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
					continue;
				}
				//Check if Cloak is allowed (Cooldown) after iCloakEffectDuration (in seconds)
				mstime now = timeInMS();
				if (mPlayerCloakData[wscCharFileName].tmUnCloakTime + mPlayerCloakData[wscCharFileName].PlayerCloakData.iUncloakEffectDuration < now && !mPlayerCloakData[wscCharFileName].bCloaked && !mPlayerCloakData[wscCharFileName].bIsCloaking && !mPlayerCloakData[wscCharFileName].bAllowCloak)
				{
					//PrintUserCmdText(iClientID, L"iUncloakEffectDuration");
					mPlayerCloakData[wscCharFileName].bAllowCloak = true;
					//PrintUserCmdText(iClientID, L"Ready to Cloak.");
				}
				

				if (mPlayerCloakData[wscCharFileName].bWantsCloak == true &&
					mPlayerWarmUpCloak[wscCharFileName].bRdy == false &&
					mPlayerCloakData[wscCharFileName].bIsCloaking == false &&
					mPlayerWarmUpCloak[wscCharFileName].msStart != 0)
				{
					//Check if CloakDevice WarmedUp
					mstime now = timeInMS();
					if ((mPlayerWarmUpCloak[wscCharFileName].msStart + mPlayerCloakData[wscCharFileName].PlayerCloakData.iCloakWarmUpDuration) < now || mPlayerCloakData[wscCharFileName].PlayerCloakData.iCloakWarmUpDuration == 0)
					{
						//PrintUserCmdText(iClientID, std::to_wstring(now));
						//PrintUserCmdText(iClientID, std::to_wstring((mPlayerWarmUpCloak[iClientID].msStart + mPlayerCloakData[wscCharFileName].PlayerCloakData.iCloakTriggerDelay)));
						mPlayerWarmUpCloak[wscCharFileName].bRdy = true;
						mPlayerCloakData[wscCharFileName].bWantsCloak = false;
						mPlayerWarmUpCloak[wscCharFileName].msStart = 0;
						mPlayerCloakData[wscCharFileName].bIsCloaking = true;

						//Start Cloak
						if (mPlayerCloakData[wscCharFileName].fCloakCap >= mPlayerCloakData[wscCharFileName].PlayerCloakData.fMinRequiredCapacityToCloak && mPlayerCloakData[wscCharFileName].bAllowCloak)
						{
							//Disable UI
							ClientController::Send_ControlMsg(false, iClientID, L"_ShowCloakEnergy");
							DoCloak(iClientID);
						}
						else {
							PrintUserCmdText(iClientID, L"Cloak is still recharging.");
							continue;
						}
						
					}
					else {
						//Keep Shield While Cloaking
						if (mPlayerCloakData[wscCharFileName].PlayerCloakData.bShieldDownOnCloaking)
						{
							KillShield(iClientID);
						}
					}
				}
			}
		}
	}

	//Cloak.
	void DoCloak(uint iClientID) {
		HK_ERROR err;



		if (!HkIsValidClientID(iClientID)) {
			return;
		}
		if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
			return;
		std::wstring wscCharFileName;
		if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
			return;
		}
		//Send MaxEnergy to Client
		int iMaxEnergy = static_cast<int>(mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity);
		ClientController::Send_ControlMsg(false, iClientID, L"MaxEnergy(" + std::to_wstring(iMaxEnergy) + L")");

		//Send MinRequiredCapacityToCloak to Client
		int iMinRequiredCapacityToCloak = static_cast<int>(mPlayerCloakData[wscCharFileName].PlayerCloakData.fMinRequiredCapacityToCloak);
		ClientController::Send_ControlMsg(false, iClientID, L"NeededEnergy(" + std::to_wstring(iMinRequiredCapacityToCloak) + L")");
		
		Cloak::mPlayerCloakData[wscCharFileName].tmCloakTime = timeInMS();
		mPlayerCloakData[wscCharFileName].bWantsCloak = false;
		mPlayerCloakData[wscCharFileName].bIsCloaking = true;
		XActivateEquip ActivateEq;
		ActivateEq.bActivate = true;
		ActivateEq.iSpaceID = ClientInfo[iClientID].iShip;
		ActivateEq.sID = Cloak::mPlayerCloakData[wscCharFileName].iCloakSlot;
		Server.ActivateEquip(iClientID, ActivateEq);

		//Check for Show UI
		if (!mPlayerCloakData[wscCharFileName].bShowUI)
		{
			mPlayerCloakData[wscCharFileName].bShowUI = true;
			ClientController::Send_ControlMsg(false, iClientID, L"_ShowCloakEnergy");
		}

		//PrintUserCmdText(iClientID, L"Cloaking.");
		ClientController::Send_ControlMsg(true, iClientID, L"_cloaktoggle");
		//ConPrint(L"toggled->on");

		Cloak::mPlayerCloakData[wscCharFileName].bCloaked = true;
		Cloak::mPlayerCloakData[wscCharFileName].bAllowUncloak = false;

	}

	//Start Cloak Process
	void StartCloakPlayer(uint iClientID) {
		HK_ERROR err;


		if (!HkIsValidClientID(iClientID)) {
			return;
		}
		if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
			return;

		std::wstring wscCharFileName;
		if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
			return;
		}
		//Check for Cloak
		if (!mPlayerCloakData[wscCharFileName].bInitialCloak)
		{
			//is Player allowed to Cloak
			if (mPlayerCloakData[wscCharFileName].bCanCloak)
			{
				//is Player not cloaked or cloaking
				if (!mPlayerCloakData[wscCharFileName].bIsCloaking || !mPlayerCloakData[wscCharFileName].bCloaked)
				{
					//GetCloakDevice Data
					bool bfound = false;
					std::list<CloakDeviceInfo>::iterator CloakDevice = lCloakDeviceList.begin();
					while (CloakDevice != lCloakDeviceList.end()) {
						if (CloakDevice->iCloakDeviceArchID == mPlayerCloakData[wscCharFileName].iCloakDeviceArchID)
						{
							bfound = true;
							break;
						}
						CloakDevice++;
					}

					//Check for Found Device
					if (!bfound)
					{ 
						//PrintUserCmdText(iClientID, L"Unknown Cloak Device");
						return;
					}


					//CloakEnergyCheck
					if (mPlayerCloakData[wscCharFileName].PlayerCloakData.fMinRequiredCapacityToCloak != 0.0)
					{
						if (mPlayerCloakData[wscCharFileName].fCloakCap <= CloakDevice->fMinRequiredCapacityToCloak)
						{
							//PrintUserCmdText(iClientID, L"Prep");

							//PrintUserCmdText(iClientID, L"Not enough CloakEnergy");
							return;
						}
					}
					else {
						mPlayerCloakData[wscCharFileName].fCloakCap = CloakDevice->fCloakCapacity;
						//PrintUserCmdText(iClientID, L"fCloakCap set");
					}

					//PrintUserCmdText(iClientID, std::to_wstring(mPlayerCloakData[wscCharFileName].fCloakCap));
					//PrintUserCmdText(iClientID, std::to_wstring(mPlayerCloakData[wscCharFileName].PlayerCloakData.fMinRequiredEnergyToCloak));

					//RdyForWarmUp
					if (CloakDevice->iCloakWarmUpDuration != 0)
					{						
						mPlayerCloakData[wscCharFileName].bIsCloaking = false;
						mPlayerCloakData[wscCharFileName].bWantsCloak = true;
						mPlayerWarmUpCloak[wscCharFileName].bRdy = false;
						mPlayerWarmUpCloak[wscCharFileName].msStart = timeInMS();
						mPlayerCloakData[wscCharFileName].PlayerCloakData = *CloakDevice;
						PrintUserCmdText(iClientID, L"Charging Cloak Device. Weapons Disabled.");		
						
						

					}
					//No WarmUP
					else {
						mPlayerCloakData[wscCharFileName].bIsCloaking = false;
						mPlayerCloakData[wscCharFileName].bWantsCloak = true;
						mPlayerWarmUpCloak[wscCharFileName].bRdy = false;
						mPlayerWarmUpCloak[wscCharFileName].msStart = timeInMS();
						mPlayerCloakData[wscCharFileName].PlayerCloakData = *CloakDevice;		
						//PrintUserCmdText(iClientID, L"Weapons Disabled.");
					}

					//Keep down Shield While Cloaking
					if (CloakDevice->bShieldDownWhileCloaking)
					{
						KillShield(iClientID);
					}
					

					
					


				}
			}
		}
	}

	//Update EnergyData
	void UpdateShipEnergyTimer() {
		HK_ERROR err;


		//CloakModule
		if (Modules::GetModuleState("CloakModule"))
		{
			struct PlayerData* pd = 0;
			while (pd = Players.traverse_active(pd)) {
				uint iClientID = HkGetClientIdFromPD(pd);		

				if (iClientID < 1 || iClientID > MAX_CLIENT_ID)
					continue;
				if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
					continue;

				uint iShip;
				pub::Player::GetShip(iClientID, iShip);
				if (!iShip) {
					continue;
				}
				std::wstring wscCharFileName;
				if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
					continue;
				}
				
				if (mPlayerCloakData[wscCharFileName].PlayerCloakData.bUseShipPowerToRecharge)
				{
					//Update EnergyData
					ClientController::Send_ControlMsg(false, iClientID, L"getpower");
				}


			}
		}
	}

	//Check for CloakWarmUp (every 250ms)
	void DoCloakingTimer250ms() {
		HK_ERROR err;


		//CloakModule
		if (Modules::GetModuleState("CloakModule"))
		{
			struct PlayerData* pd = 0;
			while (pd = Players.traverse_active(pd)) {
				uint iClientID = HkGetClientIdFromPD(pd);

				if (iClientID < 1 || iClientID > MAX_CLIENT_ID)
					continue;

				if (!HkIsValidClientID(iClientID)) {
					continue;
				}
				if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
					continue;
				
				uint iShip;
				pub::Player::GetShip(iClientID, iShip);
				if (!iShip) {
					continue;
				}
				
				std::wstring wscCharFileName;
				if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
					continue;
				}

				//Only Cloaked Players
				if (mPlayerCloakData[wscCharFileName].bCloaked)
				{
					//Sync
					Cloak::CloakSync(iClientID);

					
					//Check NeedPowerCloaked		
					if (mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakPowerUsageWhileCloaked != 0.0f)
					{
						//Check Energy Charge
						if (mPlayerCloakData[wscCharFileName].fCloakCap >= mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakPowerUsageWhileCloaked)
						{
							//Set Energy Charge
							mPlayerCloakData[wscCharFileName].fCloakCap -= mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakPowerUsageWhileCloaked;

							//Send CloakCap to Client
							int iCloakCap = static_cast<int>(mPlayerCloakData[wscCharFileName].fCloakCap);
							ClientController::Send_ControlMsg(false, iClientID, L"CloakEnergy(" + std::to_wstring(iCloakCap) + L")");
						}
						else {
							UncloakPlayer(iClientID);
						}
					}
					
					//Shield Down While Cloaking
					if (mPlayerCloakData[wscCharFileName].PlayerCloakData.bShieldDownWhileCloaking)
					{
						KillShield(iClientID);
					}
					
					//Allow Unclok after x seconds (iCloakEffectDuration)
					if (mPlayerCloakData[wscCharFileName].tmCloakTime + mPlayerCloakData[wscCharFileName].PlayerCloakData.iCloakEffectDuration < timeInMS() && mPlayerCloakData[wscCharFileName].bCloaked && mPlayerCloakData[wscCharFileName].bIsCloaking )
					{
						//PrintUserCmdText(iClientID, L"iCloakEffectDuration");
						mPlayerCloakData[wscCharFileName].bIsCloaking = false;
						mPlayerCloakData[wscCharFileName].bWantsCloak = false;
						mPlayerCloakData[wscCharFileName].bAllowUncloak = true;
						//PrintUserCmdText(iClientID, L"Uncloak now allowed!");
					}

				}
				else {
					//CheckRechage Type
					if (mPlayerCloakData[wscCharFileName].PlayerCloakData.bUseShipPowerToRecharge)
					{
						//Check Ship Energy
						if (mPlayerCloakData[wscCharFileName].fEnergy >= mPlayerCloakData[wscCharFileName].PlayerCloakData.fPowerUsageToRecharge)
						{
							//Check Energy ReCharge
							if (mPlayerCloakData[wscCharFileName].fCloakCap < mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity)
							{	
								//Calc Energy Needed
								mPlayerCloakData[wscCharFileName].fCloakCap += mPlayerCloakData[wscCharFileName].PlayerCloakData.fPowerUsageToRecharge;
								
								//Sub Energy from Ship
								int iEnergy = static_cast<int>(mPlayerCloakData[wscCharFileName].PlayerCloakData.fPowerUsageToRecharge);
								ClientController::Send_ControlMsg(false, iClientID, L"subPower(" + std::to_wstring(iEnergy) + L")");
								
								//Send CloakCap to Client
								int iCloakCap = static_cast<int>(mPlayerCloakData[wscCharFileName].fCloakCap);
								ClientController::Send_ControlMsg(false, iClientID, L"CloakEnergy(" + std::to_wstring(iCloakCap) + L")");
						
								//Update Cloak Data
								ClientController::Send_ControlMsg(false, iClientID, L"getpower");

								//Check for Show UI
								if (mPlayerCloakData[wscCharFileName].bShowUI)
								{
									if (mPlayerCloakData[wscCharFileName].fCloakCap >= mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity)
									{
										mPlayerCloakData[wscCharFileName].bShowUI = false;
										ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
									}
								}
							}
						}
			
					}
					else {
						//Check Energy ReCharge
						
						if (mPlayerCloakData[wscCharFileName].fCloakCap < mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity)
						{
							//Calc Energy Needed
							mPlayerCloakData[wscCharFileName].fCloakCap += mPlayerCloakData[wscCharFileName].PlayerCloakData.fPowerUsageToRecharge;
							if (mPlayerCloakData[wscCharFileName].fCloakCap > mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity)
							{
								mPlayerCloakData[wscCharFileName].fCloakCap = mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity;
							}
							
							//Send CloakCap to Client
							int iCloakCap = static_cast<int>(mPlayerCloakData[wscCharFileName].fCloakCap);
							ClientController::Send_ControlMsg(false, iClientID, L"CloakEnergy(" + std::to_wstring(iCloakCap) + L")");

							//Check for Show UI
							if (mPlayerCloakData[wscCharFileName].bShowUI)
							{
								if (mPlayerCloakData[wscCharFileName].fCloakCap >= mPlayerCloakData[wscCharFileName].PlayerCloakData.fCloakCapacity)
								{
									mPlayerCloakData[wscCharFileName].bShowUI = false;
									ClientController::Send_ControlMsg(false, iClientID, L"_DisableCloakEnergy");
								}
							}
						}
					}


				}
			}
		}
	}
	
	void UncloakPlayer(uint iClientID) {
		
		HK_ERROR err;


		if (!HkIsValidClientID(iClientID)) {
			return;
		}
		if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
			return;


		uint iShip;
		pub::Player::GetShip(iClientID, iShip);
		if (!iShip) {
			return;
		}
		
		std::wstring wscCharFileName;
		if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
			return;
		}
		
		if (mPlayerCloakData[wscCharFileName].bCloaked && mPlayerCloakData[wscCharFileName].bAllowUncloak) {
			ClientController::Send_ControlMsg(true, iClientID, L"_cloakoff");
			mPlayerCloakData[wscCharFileName].bIsCloaking = false;
			mPlayerCloakData[wscCharFileName].bWantsCloak = false;
			mPlayerCloakData[wscCharFileName].bCloaked = false;
			mPlayerCloakData[wscCharFileName].tmCloakTime = 0;
			mPlayerCloakData[wscCharFileName].bAllowCloak = false;
			mPlayerCloakData[wscCharFileName].tmUnCloakTime = timeInMS();
			
			//mPlayerCloakData[wscCharFileName].bInitialCloak = true;


			XActivateEquip ActivateEq;
			ActivateEq.bActivate = false;
			ActivateEq.iSpaceID = ClientInfo[iClientID].iShip;
			ActivateEq.sID = Cloak::mPlayerCloakData[wscCharFileName].iCloakSlot;
			Server.ActivateEquip(iClientID, ActivateEq);
		}
	}

	void UncloakGroup(uint iClientID) {
			
		HK_ERROR err;		

		//Get Group Members
		std::list<GROUP_MEMBER> lstMembers;
		HkGetGroupMembers((const wchar_t*)Players.GetActiveCharacterName(iClientID), lstMembers);

		//Uncloak Members
		for (auto& m : lstMembers)
		{

			std::wstring wscCharFileName;
			if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
				continue;
			}
			if (!HkIsValidClientID(iClientID)) {
				continue;
			}
			if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
				continue;
			
			if (mPlayerCloakData[wscCharFileName].bIsCloaking) {
				ClientController::Send_ControlMsg(true, m.iClientID, L"_cloakoff");
				mPlayerCloakData[wscCharFileName].bIsCloaking = false;
				mPlayerCloakData[wscCharFileName].bWantsCloak = false;
				mPlayerCloakData[wscCharFileName].bCloaked = false;
				mPlayerCloakData[wscCharFileName].tmCloakTime = 0;
			}
		}
	}

	bool Check_Dock_Call(uint iShip,uint iDockTarget,uint iCancel, enum DOCK_HOST_RESPONSE response) {

		HK_ERROR err;

		uint iClientID = HkGetClientIDByShip(iShip);
		if (iClientID) {
			// If no target then ignore the request.
			uint iTargetShip;
			pub::SpaceObj::GetTarget(iShip, iTargetShip);
			if (!iTargetShip)
				return true;

			//Check for JumpHole
			uint iType;
			pub::SpaceObj::GetType(iTargetShip, iType);
			if (iType == OBJ_JUMP_HOLE)
				return true;

			if (!HkIsValidClientID(iClientID)) {
				return true;
			}
			if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
				return true;
			
			std::wstring wscCharFileName;
			if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
				return true;
			}
			
			if (mPlayerCloakData[wscCharFileName].bCloaked)
			{
				PrintUserCmdText(iClientID, L"Docking with activated Cloak not possible.");
				pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("info_access_denied"));
				UncloakPlayer(iClientID);
				return false;
			}
			else
			{
				return true;
			}
		}	
	}

	bool Check_RequestEventFormaDocking(int iIsFormationRequest, unsigned int iShip, unsigned int iDockTarget, unsigned int p4, unsigned long p5, unsigned int iClientID)
	{
		if (iClientID)
		{
			if (!iIsFormationRequest)
			{	
				HK_ERROR err;


				//Check for JumpHole
				uint iTargetTypeID;
				pub::SpaceObj::GetType(iDockTarget, iTargetTypeID);
				if (iTargetTypeID == OBJ_JUMP_HOLE)
					return true;

				if (!HkIsValidClientID(iClientID)) {
					return true;
				}
				if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
					return true;

				std::wstring wscCharFileName;
				if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
					return true;
				}
				
				if (mPlayerCloakData[wscCharFileName].bCloaked)
				{
					PrintUserCmdText(iClientID, L"Docking with activated Cloak not possible.");
					pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("info_access_denied"));
					UncloakPlayer(iClientID);
					return false;
				}
				else
				{
					return true;
				}
			}
		}
	}

	bool Check_GoTradelane(unsigned int iClientID, struct XGoTradelane const& gtl) {
		HK_ERROR err;

		

		if (!HkIsValidClientID(iClientID)) {
			return true;
		}
		if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
			return true;

		std::wstring wscCharFileName;
		if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
			return true;
		}
		
		if (mPlayerCloakData[wscCharFileName].bCloaked || mPlayerCloakData[wscCharFileName].bIsCloaking || mPlayerCloakData[wscCharFileName].bWantsCloak || mPlayerCloakData[wscCharFileName].bCanCloak)
		{
			ClientController::Send_ControlMsg(true, iClientID, L"_cloakoff");
			mPlayerCloakData[wscCharFileName].bIsCloaking = false;
			mPlayerCloakData[wscCharFileName].bWantsCloak = false;
			mPlayerCloakData[wscCharFileName].bCloaked = false;
			mPlayerCloakData[wscCharFileName].tmCloakTime = 0;
			mPlayerCloakData[wscCharFileName].bAllowCloak = false;
			mPlayerCloakData[wscCharFileName].tmUnCloakTime = timeInMS();

			//mPlayerCloakData[wscCharFileName].bInitialCloak = true;


			XActivateEquip ActivateEq;
			ActivateEq.bActivate = false;
			ActivateEq.iSpaceID = ClientInfo[iClientID].iShip;
			ActivateEq.sID = Cloak::mPlayerCloakData[wscCharFileName].iCloakSlot;
			Server.ActivateEquip(iClientID, ActivateEq);

		}
		
		return true;
		
	}
	
	bool Check_Cloak(uint iClientID)
	{
		HK_ERROR err;



		if (!HkIsValidClientID(iClientID)) {
			return false;
		}
		if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
			return false;
		
		std::wstring wscCharFileName;
		if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
			return false;
		}

		return mPlayerCloakData[wscCharFileName].bCloaked || mPlayerCloakData[wscCharFileName].bIsCloaking;
	}

	void CloakSync(uint iClientID)
	{
		HK_ERROR err;

		
		struct PlayerData* pd = 0;
		while (pd = Players.traverse_active(pd)) {

			if (iClientID < 1 || iClientID > MAX_CLIENT_ID)
				continue;


			if (!HkIsValidClientID(iClientID)) {
				continue;
			}
			if (iClientID == -1 || HkIsInCharSelectMenu(iClientID))
				continue;

			std::wstring wscCharFileName;
			if ((err = HkGetCharFileName(ARG_CLIENTID(iClientID), wscCharFileName)) != HKE_OK) {
				continue;
			}


			if (Tools::IsPlayerInRange(iClientID, pd->iOnlineID, 20000.0f))
				
			
			//Cloak or Uncloak iClient
			//if (PlayerCloakData[pd->iOnlineID].bCloaked || PlayerCloakData[pd->iOnlineID].bIsCloaking)
			{
				XActivateEquip ActivateEq;
				ActivateEq.bActivate = true;
				ActivateEq.iSpaceID = ClientInfo[iClientID].iShip;
				ActivateEq.sID = Cloak::mPlayerCloakData[wscCharFileName].iCloakSlot;
				Server.ActivateEquip(iClientID, ActivateEq);


			}


		}

		
	}

	//KillShield if Player has Shield
	void KillShield(uint iClientID)
	{
		//Player cargo
		int iRemHoldSize;
		std::list<CARGO_INFO> lstCargo;
		HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemHoldSize);

		for (auto& cargo : lstCargo) {
			if (!cargo.bMounted)
				continue;

			//Check Archtype
			Archetype::Equipment* eq = Archetype::GetEquipment(cargo.iArchID);
			auto aType = eq->get_class_type();
			if (aType == Archetype::SHIELD || aType == Archetype::SHIELD_GENERATOR) {

				//Kill Shield
				ClientController::Send_ControlMsg(false, iClientID, L"setshield(0)");

			}
		}
	}
}