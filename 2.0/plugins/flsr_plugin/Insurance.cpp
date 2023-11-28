#include "main.h"

namespace Insurance
{
    float insuranceEquipmentCostFactor;

    struct InsuredCargoItem
    {
        CARGO_INFO CARGO_INFO;
        GoodInfo GoodInfo;
    };

    struct RestoreCargoItem
    {
        CARGO_INFO CARGO_INFO;
        float fPrice;
    };

    enum class InsuranceType : unsigned long
    {
        None,
        Mines,
        Projectiles,
        Countermeasures,
        ShieldBatteries,
        Nanobots,
        Equipment,
        All,
        Invalid
    };

    const std::map<InsuranceType, std::string> insuranceTypesStrings = {
        { InsuranceType::Countermeasures, "Countermeasures" },
        { InsuranceType::Equipment, "Equipment" },
        { InsuranceType::Mines, "Mines" },
        { InsuranceType::Nanobots, "Nanobots" },
        { InsuranceType::Projectiles, "Projectiles" },
        { InsuranceType::ShieldBatteries, "ShieldBatteries" }
    };

    const char* NANOBOT_NICKNAME = "ge_s_repair_01";
    const char* SHIELDBATTERY_NICKNAME = "ge_s_battery_01";

    const std::string AUTOINSURANCE_INI_SECTION = "AutoInsurance";
    const std::string EQUIP_PREFIX_INI_SECTION = "Equip-";
    const std::string INSURANCE_INI_SECTION = "INSURANCE";
    const std::string INSURANCE_PREFIX_INI_SECTION = "INSURANCE-";

    const std::string INSURANCE_FILE_EXTENSION = ".cfg";

    const std::string INSURANCE_ON_VALUE = "ON";
    const std::string INSURANCE_OFF_VALUE = "OFF";

    const std::string CHARACTER_NAME_KEY = "Charname";
    const std::string TOTAL_PRICE_KEY = "Worth";
    const std::string ITEMS_COUNT_KEY = "EquipCount";
    const std::string ITEM_MOUNTED_KEY = "bMounted";
    const std::string ITEM_HITPOINTS_PERCENTAGE_KEY = "fStatus";
    const std::string ITEM_HARDPOINT_KEY = "hardpoint";
    const std::string ITEM_ARCHETYPE_ID_KEY = "iArchID";
    const std::string ITEM_COUNT_KEY = "iCount";
    const std::string ITEM_ID_KEY = "iID";
    const std::string ITEM_PRICE_KEY = "fPrice";
    const std::string ITEM_ARCHETYPE_TYPE_KEY = "ClassType";

    bool IsInsuranceFileExisting(const std::string& name)
    {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }

    std::string GetHookUserFilePath(uint clientId)
    {
        CAccount* account = Players.FindAccountFromClientID(clientId);
        std::wstring accountDirectoryWS;
        if (HkGetAccountDirName(account, accountDirectoryWS) != HKE_OK)
            return "";
        return scAcctPath + wstos(accountDirectoryWS) + Globals::FLHOOKUSER_FILE;
    }

    std::set<InsuranceType> ReadEnabledInsuranceTypes(uint clientId)
    {
        const std::string hookUserFilePath = GetHookUserFilePath(clientId);

        std::wstring characterFileNameWS;
        if (hookUserFilePath.empty() || HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return std::set<InsuranceType>();
        const std::string characterFileName = wstos(characterFileNameWS);
        const std::string section = INSURANCE_PREFIX_INI_SECTION + characterFileName;

        std::set<InsuranceType> currentInsuranceTypes;
        for (const auto& insuranceTypeString : insuranceTypesStrings)
        {
            if (IniGetS(hookUserFilePath, section, insuranceTypeString.second, INSURANCE_OFF_VALUE) == INSURANCE_ON_VALUE)
                currentInsuranceTypes.insert(insuranceTypeString.first);
        }

        return currentInsuranceTypes;
    }

    bool isArchetypeTypeEquipment(Archetype::AClassType archetypeType)
    {
        return  archetypeType == Archetype::SHIELD_GENERATOR ||
                archetypeType == Archetype::THRUSTER ||
                archetypeType == Archetype::LAUNCHER ||
                archetypeType == Archetype::GUN ||
                archetypeType == Archetype::MINE_DROPPER ||
                archetypeType == Archetype::COUNTER_MEASURE_DROPPER ||
                archetypeType == Archetype::CLOAKING_DEVICE;
    }

    bool isArchetypeTypeConsumable(Archetype::AClassType archetypeType)
    {
        return archetypeType == Archetype::MINE ||
               archetypeType == Archetype::MUNITION ||
               archetypeType == Archetype::COUNTER_MEASURE ||
               archetypeType == Archetype::SHIELD_BATTERY ||
               archetypeType == Archetype::REPAIR_KIT;
    }

    int CalculateInsuranceCost(std::list<InsuredCargoItem> insuredCargoList)
    {
        float totalPrice = 0.0f;
        for (auto const& cargo : insuredCargoList)
        {
            float price = cargo.GoodInfo.fPrice * cargo.CARGO_INFO.iCount;
            Archetype::Equipment const* equipment = Archetype::GetEquipment(cargo.CARGO_INFO.iArchID);
            const auto archetypeType = equipment->get_class_type();
            totalPrice += isArchetypeTypeEquipment(archetypeType) ? price * insuranceEquipmentCostFactor : price;
        }
        return static_cast<int>(std::max(0.0f, totalPrice));
    }

    bool isEquipmentInsuranceActive(uint clientId)
    {
        const auto insuredTypes = ReadEnabledInsuranceTypes(clientId);
        return insuredTypes.contains(InsuranceType::Equipment);
    }

    bool isConsumableInsuranceActive(Archetype::AClassType archetypeType, uint clientId)
    {
        const auto insuredTypes = ReadEnabledInsuranceTypes(clientId);
        return  insuredTypes.contains(InsuranceType::Mines) && archetypeType == Archetype::MINE ||
                insuredTypes.contains(InsuranceType::Projectiles) && archetypeType == Archetype::MUNITION ||
                insuredTypes.contains(InsuranceType::Countermeasures) && archetypeType == Archetype::COUNTER_MEASURE ||
                insuredTypes.contains(InsuranceType::ShieldBatteries) && archetypeType == Archetype::SHIELD_BATTERY ||
                insuredTypes.contains(InsuranceType::Nanobots) && archetypeType == Archetype::REPAIR_KIT;
    }

    std::list<CARGO_INFO> getCargoList(uint clientId)
    {
        int remainingHoldSize;
        std::list<CARGO_INFO> cargoList;
        if (HkEnumCargo(ARG_CLIENTID(clientId), cargoList, remainingHoldSize) != HKE_OK)
            return std::list<CARGO_INFO>();
        return cargoList;
    }

    std::list<InsuredCargoItem> CollectInsuredCargo(uint clientId, bool onlyFreeItems)
    {
        std::list<InsuredCargoItem> insuredCargoList;
        const std::list<CARGO_INFO> cargoList = getCargoList(clientId);
        for (auto const& cargo : cargoList)
        {
            if (cargo.bMission)
                continue;

            Archetype::Equipment const* equipment = Archetype::GetEquipment(cargo.iArchID);
            auto archetypeType = equipment->get_class_type();

            if (cargo.bMounted && isArchetypeTypeEquipment(archetypeType) && (onlyFreeItems || isEquipmentInsuranceActive(clientId)))
            {
                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi)
                {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi)
                    {
                        if (onlyFreeItems && gi->fPrice > 0.0f)
                            continue;
                        InsuredCargoItem insuredCargo;
                        insuredCargo.CARGO_INFO = cargo;
                        insuredCargo.GoodInfo = *gi;
                        insuredCargoList.push_back(insuredCargo);
                    }
                }
            }
            else if (!onlyFreeItems && isConsumableInsuranceActive(archetypeType, clientId))
            {
                uint nanobotId;
                pub::GetGoodID(nanobotId, NANOBOT_NICKNAME);
                uint shieldBatteryId;
                pub::GetGoodID(shieldBatteryId, SHIELDBATTERY_NICKNAME);
                uint maxNanobots = std::numeric_limits<uint>::max();
                uint maxShieldBatteries = std::numeric_limits<uint>::max();
                const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
                if (ship)
                {
                    maxNanobots = ship->iMaxNanobots;
                    maxShieldBatteries = ship->iMaxShieldBats;
                }

                const GoodInfo* gi = GoodList_get()->find_by_id(cargo.iArchID);
                if (gi)
                {
                    gi = GoodList::find_by_id(gi->iArchID);
                    if (gi)
                    {
                        InsuredCargoItem insuredCargo;
                        insuredCargo.CARGO_INFO = cargo;
                        insuredCargo.GoodInfo = *gi;
                        if ((gi->iArchID == nanobotId) && (static_cast<uint>(cargo.iCount) > maxNanobots))
                        {
                            CARGO_INFO insured_NB = cargo;
                            insured_NB.iCount = maxNanobots;
                            insuredCargo.CARGO_INFO = insured_NB;
                        }
                        if ((gi->iArchID == shieldBatteryId) && (static_cast<uint>(cargo.iCount) > maxShieldBatteries))
                        {
                            CARGO_INFO insured_SB = cargo;
                            insured_SB.iCount = maxShieldBatteries;
                            insuredCargo.CARGO_INFO = insured_SB;
                        }
                        insuredCargoList.push_back(insuredCargo);
                    }
                }
            }
        }
        return insuredCargoList;
    }

    void CreateNewInsurance(uint clientId, bool onlyFreeItems)
    {
        int playerCash = 0;
        if (!onlyFreeItems)
        {
            if (HK_ERROR error = HkGetCash(ARG_CLIENTID(clientId), playerCash); error != HKE_OK)
            {
                PrintUserCmdText(clientId, L"ERR Get cash failed err=" + HkErrGetText(error));
                return;
            }
        }

        std::list<InsuredCargoItem> insuredCargoList = CollectInsuredCargo(clientId, onlyFreeItems);
        if (!onlyFreeItems && (insuredCargoList.size() == 0))
        {
            PrintUserCmdText(clientId, L"You own nothing that needs to be insured.");
            return;
        }

        int totalPrice = !onlyFreeItems ? CalculateInsuranceCost(insuredCargoList) : 0;

        if (totalPrice > playerCash)
        {
            PrintUserCmdText(clientId, L"Insurance failed. You need to own at least $" + ToMoneyStr(totalPrice) + L".");
            // If the player has not enough money, proceed with only free items which always must be insured.
            totalPrice = 0;
            onlyFreeItems = true;
            insuredCargoList = CollectInsuredCargo(clientId, onlyFreeItems);
            if (insuredCargoList.size() == 0)
                return;
        }

        if (totalPrice > 0)
        {
            if (HK_ERROR error = HkAddCash(ARG_CLIENTID(clientId), -totalPrice); error != HKE_OK)
            {
                PrintUserCmdText(clientId, L"ERR Remove cash failed err=" + HkErrGetText(error));
                return;
            }
        }

        if (HkAntiCheat(clientId) != HKE_OK)
        {
            PrintUserCmdText(clientId, L"ERR Insurance-Booking failed");
            const std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
            AddLog("NOTICE: Possible cheating when book Insurance %s credits from %s ", wstos(ToMoneyStr(totalPrice)).c_str(), wstos(characterNameWS).c_str());
            return;
        }

        char currentDirectory[MAX_PATH];
        GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
        const std::string insurancesDirectory = std::string(currentDirectory) + Globals::INSURANCE_STORE;
        std::wstring characterFileNameWS;
        if (HK_ERROR error = HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS); error != HKE_OK)
        {
            PrintUserCmdText(clientId, L"ERR Get cash failed err=" + HkErrGetText(error));
            return;
        }
        const std::string characterFileName = wstos(characterFileNameWS);
        const std::string insuranceFilePath = insurancesDirectory + characterFileName + INSURANCE_FILE_EXTENSION;

        IniWrite(insuranceFilePath, INSURANCE_INI_SECTION, CHARACTER_NAME_KEY, characterFileName);
        IniWrite(insuranceFilePath, INSURANCE_INI_SECTION, TOTAL_PRICE_KEY, std::to_string(totalPrice));
        IniWrite(insuranceFilePath, INSURANCE_INI_SECTION, ITEMS_COUNT_KEY, std::to_string(insuredCargoList.size()));

        std::list<InsuredCargoItem>::iterator insuredCargo = insuredCargoList.begin();
        int equipmentIndex = 0;
        for (auto const& insuredCargo : insuredCargoList)
        {
            const std::string section = EQUIP_PREFIX_INI_SECTION + std::to_string(equipmentIndex++);
            const GoodInfo gi = insuredCargo.GoodInfo;
            const CARGO_INFO ci = insuredCargo.CARGO_INFO;
            IniWrite(insuranceFilePath, section, ITEM_MOUNTED_KEY, std::to_string(ci.bMounted));
            IniWrite(insuranceFilePath, section, ITEM_HITPOINTS_PERCENTAGE_KEY, std::to_string(ci.fStatus));
            IniWrite(insuranceFilePath, section, ITEM_HARDPOINT_KEY, ci.hardpoint.value);
            IniWrite(insuranceFilePath, section, ITEM_ARCHETYPE_ID_KEY, std::to_string(ci.iArchID));
            IniWrite(insuranceFilePath, section, ITEM_COUNT_KEY, std::to_string(ci.iCount));
            IniWrite(insuranceFilePath, section, ITEM_ID_KEY, std::to_string(ci.iID));
            Archetype::Equipment const* equipment = Archetype::GetEquipment(ci.iArchID);
            const auto archetypeType = equipment->get_class_type();
            const float price = isArchetypeTypeConsumable(archetypeType) ? gi.fPrice : gi.fPrice * insuranceEquipmentCostFactor;
            IniWrite(insuranceFilePath, section, ITEM_PRICE_KEY, std::to_string(price));
            IniWrite(insuranceFilePath, section, ITEM_ARCHETYPE_TYPE_KEY, std::to_string(static_cast<int>(archetypeType)));
        }

        if (onlyFreeItems)
        {
            PrintUserCmdText(clientId, L"Pre-mounted equipment is insured.");
        }
        else
        {
            PrintUserCmdText(clientId, L"Your ship is insured with a worth of $" + ToMoneyStr(totalPrice) + L". Unspent insurance deposit will be returned on landing.");
        }
    }

    void UseInsurance(uint clientId)
    {
        char currentDirectory[MAX_PATH];
        GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
        std::string insurancesDirectory = std::string(currentDirectory) + Globals::INSURANCE_STORE;
        std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;
        std::string characterFileName = wstos(characterFileNameWS);
        std::string insuranceFilePath = insurancesDirectory + characterFileName + INSURANCE_FILE_EXTENSION;

        if (!IsInsuranceFileExisting(insuranceFilePath))
            return;

        int insuredEquipmentCount = IniGetI(insuranceFilePath, INSURANCE_INI_SECTION, ITEMS_COUNT_KEY, 0);

        std::list<RestoreCargoItem> insuredCargoList;
        for (int equipmentIndex = 0; equipmentIndex < insuredEquipmentCount; equipmentIndex++)
        {
            const std::string section = EQUIP_PREFIX_INI_SECTION + std::to_string(equipmentIndex);

            CARGO_INFO cargoInfo;
            cargoInfo.bMission = false;
            cargoInfo.bMounted = IniGetB(insuranceFilePath, section, ITEM_MOUNTED_KEY, true);
            cargoInfo.fStatus = IniGetF(insuranceFilePath, section, ITEM_HITPOINTS_PERCENTAGE_KEY, 1.0f);
            CacheString hardpoint;
            hardpoint.value = StringAlloc(IniGetS(insuranceFilePath, section, ITEM_HARDPOINT_KEY, "").c_str(), false);
            cargoInfo.hardpoint = hardpoint;
            cargoInfo.iArchID = IniGetI(insuranceFilePath, section, ITEM_ARCHETYPE_ID_KEY, 0);
            cargoInfo.iCount = IniGetI(insuranceFilePath, section, ITEM_COUNT_KEY, 0);
            cargoInfo.iID = IniGetI(insuranceFilePath, section, ITEM_ID_KEY, 0);

            RestoreCargoItem insuredCargo;
            insuredCargo.CARGO_INFO = cargoInfo;
            insuredCargo.fPrice = IniGetF(insuranceFilePath, section, ITEM_PRICE_KEY, 0);
            insuredCargoList.push_back(insuredCargo);
        }

        std::list<CARGO_INFO> currentCargoList = getCargoList(clientId);
        std::list<CARGO_INFO> filteredCurrentCargoList;
        for (auto const& cargo : currentCargoList)
        {
            Archetype::Equipment const* equipment = Archetype::GetEquipment(cargo.iArchID);
            const auto archetypeType = equipment->get_class_type();

            if (cargo.bMounted && isArchetypeTypeEquipment(archetypeType))
                filteredCurrentCargoList.push_back(cargo);
            else if (isArchetypeTypeConsumable(archetypeType))
                filteredCurrentCargoList.push_back(cargo);
        }

        bool somethingWasRestored = false;
        bool consumablesCouldNotBeRestored = false;
        float totalRestorationPrice = 0.0f;
        for (auto const& insuredCargo : insuredCargoList)
        {
            Archetype::Equipment const* equipment = Archetype::GetEquipment(insuredCargo.CARGO_INFO.iArchID);
            const auto archetypeType = equipment->get_class_type();
            int foundCurrentCargoCount = 0;
            if (isArchetypeTypeEquipment(archetypeType))
            {
                for (auto const& currentCargo : filteredCurrentCargoList)
                {
                    if (insuredCargo.CARGO_INFO.hardpoint.value == currentCargo.hardpoint.value)
                    {
                        foundCurrentCargoCount = currentCargo.iCount;
                        break;
                    }
                }
            }
            else
            {
                for (auto const& currentCargo : filteredCurrentCargoList)
                {
                    if (currentCargo.iArchID == insuredCargo.CARGO_INFO.iArchID)
                    {
                        foundCurrentCargoCount = currentCargo.iCount;
                        break;
                    }
                }
            }

            if (isArchetypeTypeConsumable(archetypeType))
            {
                float remainingHoldSize;
                pub::Player::GetRemainingHoldSize(clientId, remainingHoldSize);
                const uint itemsToAdd = insuredCargo.CARGO_INFO.iCount - foundCurrentCargoCount;
                const uint maxItemsToAdd = equipment->fVolume > 0.0f ? static_cast<uint>(remainingHoldSize / equipment->fVolume) : itemsToAdd;
                const uint actualItemsToAdd = std::min(itemsToAdd, maxItemsToAdd);
                consumablesCouldNotBeRestored = consumablesCouldNotBeRestored || actualItemsToAdd != itemsToAdd;
                if ((actualItemsToAdd > 0) && (Tools::FLSRHkAddCargo(characterNameWS, insuredCargo.CARGO_INFO.iArchID, actualItemsToAdd, false) == HKE_OK))
                {
                    totalRestorationPrice += actualItemsToAdd * insuredCargo.fPrice;
                    somethingWasRestored = true;
                }
            }
            else if (isArchetypeTypeEquipment(archetypeType) && foundCurrentCargoCount == 0)
            {
                std::string hardpoint = insuredCargo.CARGO_INFO.hardpoint.value;
                if (HkAddEquip(ARG_CLIENTID(clientId), insuredCargo.CARGO_INFO.iArchID, hardpoint) == HKE_OK)
                {
                    totalRestorationPrice += insuredCargo.fPrice;
                    somethingWasRestored = true;
                    // Anti Cheat
                    char* szClassPtr;
                    memcpy(&szClassPtr, &Players, 4);
                    szClassPtr += 0x418 * (clientId - 1);
                    ulong lCRC;
                    __asm
                    {
                        pushad
                        mov ecx, [szClassPtr]
                        call[CRCAntiCheat_FLSR]
                        mov[lCRC], eax
                        popad
                    }
                    memcpy(szClassPtr + 0x320, &lCRC, 4);
                }
            }
        }

        int insuredTotalPrice = IniGetI(insuranceFilePath, INSURANCE_INI_SECTION, TOTAL_PRICE_KEY, 0);
        int moneyBack = insuredTotalPrice - static_cast<int>(totalRestorationPrice);
        if (HK_ERROR error = HkAddCash(characterNameWS, std::max(0, moneyBack)); error != HKE_OK)
        {
            PrintUserCmdText(clientId, L"ERR Add cash failed err=" + HkErrGetText(error));
        }

        std::wstring restorationOutput = L"";
        if (somethingWasRestored)
            restorationOutput += L"Your insured items were restored. ";
        if (consumablesCouldNotBeRestored)
            restorationOutput += L"Some consumables could not be restored due to full cargo hold. ";
        if (moneyBack > 0)
            restorationOutput += L"Unspent $" + ToMoneyStr(moneyBack) + L" insurance deposit were returned.";
        if (!restorationOutput.empty())
            PrintUserCmdText(clientId, restorationOutput);

        // Clear insurance after usage.
        remove(insuranceFilePath.c_str());
    }

    bool IsInsuranceRequested(uint clientId)
    {
        const std::wstring characterNameWS = (wchar_t*)Players.GetActiveCharacterName(clientId);
        const std::string characterName = wstos(characterNameWS);
        std::wstring characterFileNameWS;
        if (HK_ERROR error = HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS); error != HKE_OK)
            return false;
        const std::string characterFileName = wstos(characterFileNameWS);
        const std::string hookUserFilePath = GetHookUserFilePath(clientId);
        return IniGetS(hookUserFilePath, INSURANCE_PREFIX_INI_SECTION + characterFileName, AUTOINSURANCE_INI_SECTION, INSURANCE_OFF_VALUE) == INSURANCE_ON_VALUE;
    }

    bool IsInsurancePresent(uint clientId)
    {
        char currentDirectory[MAX_PATH];
        GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
        std::string insurancesDirectory = std::string(currentDirectory) + Globals::INSURANCE_STORE;
        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return false;
        std::string characterFileName = wstos(characterFileNameWS);
        std::string insuranceFilePath = insurancesDirectory + characterFileName + INSURANCE_FILE_EXTENSION;
        return IsInsuranceFileExisting(insuranceFilePath);
    }

    InsuranceType GetInsuranceTypeFromString(const std::string& value)
    {
        const std::string lowerValue = ToLower(value);

        if (lowerValue == "none" || lowerValue == "off")
        {
            return InsuranceType::None;
        }
        else if (lowerValue == "mines" || lowerValue == "mine" || lowerValue == "m")
        {
            return InsuranceType::Mines;
        }
        else if (lowerValue == "projectiles" || lowerValue == "pj" || lowerValue == "p" ||
            lowerValue == "rocket" || lowerValue == "rockets" || lowerValue == "torpedo" ||
            lowerValue == "torp" || lowerValue == "torps" || lowerValue == "missiles" ||
            lowerValue == "missile")
        {
            return InsuranceType::Projectiles;
        }
        else if (lowerValue == "countermeasures" || lowerValue == "cm" || lowerValue == "cms")
        {
            return InsuranceType::Countermeasures;
        }
        else if (lowerValue == "shieldbatteries" || lowerValue == "shield" || lowerValue == "shieldbats" ||
            lowerValue == "bats" || lowerValue == "sb" || lowerValue == "s")
        {
            return InsuranceType::ShieldBatteries;
        }
        else if (lowerValue == "nanobots" || lowerValue == "bots" || lowerValue == "nanos" ||
            lowerValue == "nb" || lowerValue == "n")
        {
            return InsuranceType::Nanobots;
        }
        else if (lowerValue == "equipment" || lowerValue == "equip" || lowerValue == "e")
        {
            return InsuranceType::Equipment;
        }
        else if (lowerValue == "all")
        {
            return InsuranceType::All;
        }

        return InsuranceType::Invalid;
    }

    std::string GetCurrentlyInsuredTypesJoinedString(uint clientId)
    {
        const std::set<InsuranceType> currentInsuranceTypes = ReadEnabledInsuranceTypes(clientId);
        std::string insuranceTypesString = "";
        if (currentInsuranceTypes.empty())
        {
            insuranceTypesString = "Nothing";
        }
        else if (currentInsuranceTypes.size() == insuranceTypesStrings.size())
        {
            insuranceTypesString = "Equipment and Consumables";
        }
        else
        {
            uint index = 0;
            for (const auto& insuranceType : currentInsuranceTypes)
            {
                const auto foundInsuranceTypeString = insuranceTypesStrings.find(insuranceType);
                if (foundInsuranceTypeString != insuranceTypesStrings.end())
                {
                    insuranceTypesString += foundInsuranceTypeString->second;
                    if (++index < currentInsuranceTypes.size())
                        insuranceTypesString += ", ";
                }
            }
        }
        return insuranceTypesString;
    }

    void UserCMD_INSURANCE(uint clientId, const std::wstring& argumentsWS)
    {
        if (!Modules::GetModuleState("InsuranceModule"))
            return;

        std::wstring characterFileNameWS;
        HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS);
        const std::string characterFileName = wstos(characterFileNameWS);

        const std::set<InsuranceType> currentInsuranceTypes = ReadEnabledInsuranceTypes(clientId);

        const std::string& arguments = wstos(GetParamToEnd(argumentsWS, ' ', 0));

        // When there are no arguments, output general statistics about the current insurance situation and possible options.
        if (arguments.empty())
        {
            PrintUserCmdText(clientId, L"Currently insured: " + stows(GetCurrentlyInsuredTypesJoinedString(clientId)));

            uint ship;
            pub::Player::GetShip(clientId, ship);
            if (ship)
            {
                if (!currentInsuranceTypes.empty())
                {
                    char currentDirectory[MAX_PATH];
                    GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
                    const std::string insuranceFilePath = std::string(currentDirectory) + Globals::INSURANCE_STORE + characterFileName + INSURANCE_FILE_EXTENSION;
                    const bool insuranceFileExists = IsInsuranceFileExisting(insuranceFilePath);
                    if (insuranceFileExists)
                    {
                        int storeInsuranceWorth = IniGetI(insuranceFilePath, INSURANCE_INI_SECTION, TOTAL_PRICE_KEY, 0);
                        PrintUserCmdText(clientId, L"Current insured worth: $" + ToMoneyStr(storeInsuranceWorth));
                    }
                }
            }
            else
            {
                PrintUserCmdText(clientId, L"Possible insurances: None, All, Equipment, Nanobots, ShieldBatteries, Countermeasures, Projectiles, Mines");
                PrintUserCmdText(clientId, L"Estimated value of an insurance: $" + ToMoneyStr(CalculateInsuranceCost(CollectInsuredCargo(clientId, false))));
            }
        }
        // When there are arguments, toggle insurance features accordingly.
        else
        {
            uint ship;
            pub::Player::GetShip(clientId, ship);
            if (ship)
            {
                PrintUserCmdText(clientId, L"You can only apply for an insurance when docked.");
                return;
            }

            const std::string hookUserFilePath = GetHookUserFilePath(clientId);
            const std::string section = INSURANCE_PREFIX_INI_SECTION + characterFileName;

            // Separate arguments by a space character.
            std::istringstream iss(arguments);
            std::vector<std::string> separatedArguments{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };
            std::set<InsuranceType> inputInsuranceTypes;
            for (const auto& argument : separatedArguments)
                inputInsuranceTypes.insert(GetInsuranceTypeFromString(argument));

            bool insuranceActivated = false;
            // Determine the new state of every insurance type based on the current arguments and previously active insurance types.
            for (const auto& insuranceTypeString : insuranceTypesStrings)
            {
                bool newState;
                if (inputInsuranceTypes.contains(InsuranceType::All))
                    newState = true;
                else if (inputInsuranceTypes.contains(InsuranceType::None))
                    newState = false;
                else if (inputInsuranceTypes.contains(insuranceTypeString.first))
                    newState = currentInsuranceTypes.contains(insuranceTypeString.first) != inputInsuranceTypes.contains(insuranceTypeString.first);
                else
                    newState = currentInsuranceTypes.contains(insuranceTypeString.first);

                IniWrite(hookUserFilePath, section, insuranceTypeString.second, newState ? INSURANCE_ON_VALUE : INSURANCE_OFF_VALUE);
                insuranceActivated = insuranceActivated || newState;
            }

            IniWrite(hookUserFilePath, section, AUTOINSURANCE_INI_SECTION, insuranceActivated ? INSURANCE_ON_VALUE : INSURANCE_OFF_VALUE);

            PrintUserCmdText(clientId, L"Currently insured: " + stows(GetCurrentlyInsuredTypesJoinedString(clientId)));
        }
    }
}
