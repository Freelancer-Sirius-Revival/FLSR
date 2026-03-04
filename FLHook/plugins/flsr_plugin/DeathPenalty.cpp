#include "Main.h"
#include "DeathPenalty.h"
#include "Plugin.h"

namespace DeathPenalty
{
    float penaltyCostFactor = 1.0f;
    float shipRepairCostFactor = 0.33f;
    float equipmentRepairCostFactor = 0.3f;
    const std::string IniSectionHeading  = "DeathPenalties";

    static std::string GetDeathPenaltyMapFilePath()
    {
        return scAcctPath + "\\deathpenalty.ini";
    }

    std::unordered_map<std::string, int> deathPenaltyByCharacterFileName;
    std::unordered_set<uint> excludedSystemIds;

    static void LoadSettings()
    {
        char currentDirectory[MAX_PATH];
        GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
        const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-DeathPenalty.ini";

        excludedSystemIds.clear();
        INI_Reader ini;
        if (ini.open(configFilePath.c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("General"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("cost_factor"))
                            penaltyCostFactor = ini.get_value_float(0);
                        if (ini.is_value("excluded_system"))
                            excludedSystemIds.insert(CreateID(ini.get_value_string(0)));
                    }
                }
            }
            ini.close();
        }
    }

    static void ReadCharacterData()
    {
        deathPenaltyByCharacterFileName.clear();
        INI_Reader ini;
        if (ini.open(GetDeathPenaltyMapFilePath().c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header(IniSectionHeading.c_str()))
                {
                    while (ini.read_value())
                    {
                        const std::string line = ini.get_line_ptr();
                        const auto pos = line.find('=');
                        if (pos != std::string::npos)
                        {
                            const std::string& characterFileName = Trim(line.substr(0, pos - 1));
                            deathPenaltyByCharacterFileName[characterFileName] = ini.get_value_int(1);
                        }
                    }
                }
            }
            ini.close();
        }
    }

    bool initialized = false;
    void Initialize()
    {
        if (initialized)
            return;
        initialized = true;

        ConPrint(L"Initializing Death Penalty... ");

        LoadSettings();
        ReadCharacterData();

        const HMODULE commonHandle = GetModuleHandle("common.dll");
        if (commonHandle)
            shipRepairCostFactor = *(float*)(DWORD(commonHandle) + 0x004A28);

        const HMODULE freelancerHandle = GetModuleHandle("freelancer.exe");
        if (freelancerHandle)
            equipmentRepairCostFactor = *(float*)(DWORD(freelancerHandle) + 0x1D51B4);

        ConPrint(L"Done\n");
    }

    static std::string GetCharacterFileName(const uint clientId)
    {
        if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return "";
        std::wstring characterFileName;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileName) != HKE_OK)
            return "";
        return wstos(characterFileName);
    }

    static bool IsEquipment(const Archetype::AClassType archetypeType)
    {
        return archetypeType == Archetype::SHIELD_GENERATOR ||
            archetypeType == Archetype::THRUSTER ||
            archetypeType == Archetype::LAUNCHER ||
            archetypeType == Archetype::GUN ||
            archetypeType == Archetype::MINE_DROPPER ||
            archetypeType == Archetype::COUNTER_MEASURE_DROPPER;
    }

    static void PersistDeathPenalty(const std::string& characterFileName, const int cash)
    {
        deathPenaltyByCharacterFileName[characterFileName] = cash;
        IniWrite(GetDeathPenaltyMapFilePath(), IniSectionHeading, characterFileName, std::to_string(cash));
    }

    static void ClearDeathPenalty(const std::string& characterFileName)
    {
        deathPenaltyByCharacterFileName.erase(characterFileName);
        IniDelete(GetDeathPenaltyMapFilePath(), IniSectionHeading, characterFileName);
    }

    static std::wstring PrintMoney(const int64_t amount)
    {
        std::wstring result = std::to_wstring(amount);
        for (int pos = result.size() - 3; pos > 0; pos = pos - 3)
            result = result.insert(pos, L",");
        return L"$" + result;
    }

    static int GetMaxShipHullAndCollGroupsRepairCost(const uint clientId)
    {
        const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
        const float shipMaxHitpoints = ship->fHitPoints;
        int repairCost = static_cast<int>(shipMaxHitpoints * shipRepairCostFactor);

        Archetype::CollisionGroup* archCollGroup = ship->collisiongroup;
        while (archCollGroup)
        {
            repairCost += static_cast<int>(std::round(archCollGroup->hitPts * shipRepairCostFactor));
            archCollGroup = archCollGroup->next;
        }
        return repairCost;
    }

    static int CalculateDeathPenalty(const uint clientId)
    {
        int totalCost = GetMaxShipHullAndCollGroupsRepairCost(clientId);

        for (const auto& equip : Players[clientId].equipDescList.equip)
        {
            if (equip.bMission || !equip.bMounted)
                continue;

            const Archetype::Equipment* archetype = Archetype::GetEquipment(equip.get_arch_id());
            const GoodInfo* good = GoodList::find_by_id(equip.get_arch_id());
            if (!archetype || !good)
                continue;

            const Archetype::AClassType archetypeType = archetype->get_class_type();
            if (IsEquipment(archetypeType))
                totalCost += static_cast<int>(std::floor(good->fPrice * equipmentRepairCostFactor));
        }

        totalCost *= penaltyCostFactor;
        return totalCost <= 0 ? 0 : totalCost;
    }

    static void ApplyDeathPenalty(const int clientId)
    {
        const auto& characterFileName = GetCharacterFileName(clientId);
        if (characterFileName.empty() || !deathPenaltyByCharacterFileName.contains(characterFileName))
            return;

        int playerCash = 0;
        if (HK_ERROR error = HkGetCash(ARG_CLIENTID(clientId), playerCash); error != HKE_OK)
        {
            PrintUserCmdText(clientId, L"ERR Get cash failed err=" + HkErrGetText(error));
            return;
        }

        const int remainingCost = std::min(playerCash, deathPenaltyByCharacterFileName[characterFileName]);
        if (remainingCost > 0)
        {
            if (HK_ERROR error = HkAddCash(ARG_CLIENTID(clientId), remainingCost * -1); error == HKE_OK)
                PrintUserCmdText(clientId, L"You were charged " + PrintMoney(remainingCost) + L" for dying.");
            else
                PrintUserCmdText(clientId, L"ERR Add cash failed err=" + HkErrGetText(error));
        }
    }

    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId)
    {
        std::wstring characterFileName;
        if (HkGetCharFileName(info.wszCharname, characterFileName) == HKE_OK)
            ClearDeathPenalty(wstos(characterFileName));
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId)
    {
        const std::string characterFileName = std::string(characterId.charFilename).substr(0, 11);
        ClearDeathPenalty(characterFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId)
    {
        const std::string& characterFileName = GetCharacterFileName(clientId);
        if (!characterFileName.empty())
            ClearDeathPenalty(characterFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall BaseExit_After(unsigned int baseId, unsigned int clientId)
    {
        const int totalCost = CalculateDeathPenalty(clientId);
        if (totalCost > 0)
        {
            const auto& characterFileName = GetCharacterFileName(clientId);
            if (!characterFileName.empty())
            {
                PersistDeathPenalty(characterFileName, totalCost);
                std::wstring message = L"Upon death you will be charged " + PrintMoney(totalCost) + L".";
                uint systemId = 0;
                pub::Player::GetSystem(clientId, systemId);
                if (systemId && excludedSystemIds.contains(systemId))
                    message += L" This star system is excluded from death penalty.";
                PrintUserCmdText(clientId, message);
            }
        }

        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerId)
    {
        if (killed && killedObject->is_player())
        {
            const uint clientId = killedObject->cobj->ownerPlayer;
            if (!excludedSystemIds.contains(killedObject->cobj->system))
                ApplyDeathPenalty(clientId);
            else
                PrintUserCmdText(clientId, L"No money has been charged for dying in this star system.");
            const std::string& characterFileName = GetCharacterFileName(clientId);
            if (!characterFileName.empty())
                ClearDeathPenalty(characterFileName);
        }

        returncode = DEFAULT_RETURNCODE;
    }

    std::wstring oldCharacterFileName = L"";

    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        std::wstring output = L"";
        oldCharacterFileName = HkGetCharFileName(charname, output) == HKE_OK ? output : L"";
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }

    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        const std::string& sOldCharacterFileName = wstos(oldCharacterFileName);
        if (!oldCharacterFileName.empty() && deathPenaltyByCharacterFileName.contains(sOldCharacterFileName))
        {
            if (onlyDelete)
            {
                ClearDeathPenalty(sOldCharacterFileName);
            }
            else if (std::wstring newCharacterFileName; HkGetCharFileName(newCharname, newCharacterFileName) == HKE_OK)
            {
                const std::string& sNewCharacterFileName = wstos(newCharacterFileName);
                PersistDeathPenalty(sNewCharacterFileName, deathPenaltyByCharacterFileName[sOldCharacterFileName]);
                ClearDeathPenalty(sOldCharacterFileName);
            }
        }

        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }

    bool UserCmds(const uint clientId, const std::wstring& argumentsWS)
    {
        if (ToLower(argumentsWS).find(L"/deathpenalty") == 0 || ToLower(argumentsWS).find(L"/dp") == 0)
        {
            const auto& characterFileName = GetCharacterFileName(clientId);
            if (!characterFileName.empty() && deathPenaltyByCharacterFileName.contains(characterFileName))
                PrintUserCmdText(clientId, L"Upon death you will be charged " + PrintMoney(deathPenaltyByCharacterFileName[characterFileName]) + L".");

            returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            return true;
        }
        returncode = DEFAULT_RETURNCODE;
        return false;
    }

    bool ExecuteCommandString(CCmds* cmds, const std::wstring& wscCmd)
    {
        if (IS_CMD("dp"))
        {
            const uint clientId = ((CInGame*)cmds)->iClientID;
            if (!(cmds->rights & CCMDS_RIGHTS::RIGHT_SUPERADMIN))
            {
                PrintUserCmdText(clientId, L"ERR No permission");
                returncode = SKIPPLUGINS_NOFUNCTIONCALL;
                return false;
            }

            const std::string targetNickname = wstos(ToLower(cmds->ArgStr(1)));
            if (!targetNickname.empty())
            {
                const uint systemId = CreateID(targetNickname.c_str());
                if (excludedSystemIds.contains(systemId))
                {
                    excludedSystemIds.erase(systemId);
                    PrintUserCmdText(clientId, L"Death Penalty enabled for " + stows(targetNickname));
                }
                else
                {
                    excludedSystemIds.insert(systemId);
                    PrintUserCmdText(clientId, L"Death Penalty disabled for " + stows(targetNickname));
                }
            }

            returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            return true;
        }

        returncode = DEFAULT_RETURNCODE;
        return false;
    }
}
