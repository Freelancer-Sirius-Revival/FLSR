#include "Insurance.h"
#include "Plugin.h"
#include <filesystem>

namespace Insurance
{
    std::string outputDirectory;
    float shipRepairCostFactor = 0.33f;
    float equipmentRepairCostFactor = 0.3f;

    bool initialized = false;
    void Initialize()
    {
        if (initialized)
            return;
        initialized = true;

        ConPrint(L"Initializing Insurance... ");

        const HMODULE commonHandle = GetModuleHandle("common.dll");
        if (commonHandle)
            shipRepairCostFactor = *(float*)(DWORD(commonHandle) + 0x004A28);

        const HMODULE freelancerHandle = GetModuleHandle("freelancer.exe");
        if (freelancerHandle)
            equipmentRepairCostFactor = *(float*)(DWORD(freelancerHandle) + 0x1D51B4);

        // Set up the directory name for all save files of this plugin.
        outputDirectory = scAcctPath + "\\insurances\\";

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

    static std::string GetInsuranceFilePath(const uint clientId)
    {
        const std::string charFileName = GetCharacterFileName(clientId);
        return charFileName.empty() ? "" : outputDirectory + charFileName + ".ini";
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

    static bool IsConsumable(const Archetype::AClassType archetypeType)
    {
        return archetypeType == Archetype::MINE ||
               archetypeType == Archetype::MUNITION ||
               archetypeType == Archetype::COUNTER_MEASURE ||
               archetypeType == Archetype::SHIELD_BATTERY ||
               archetypeType == Archetype::REPAIR_KIT;
    }

    enum InsuranceType : uchar
    {
        None = 0,
        Equipment = 1 << 0,
        Consumables = 1 << 1,
        All = Equipment | Consumables
    };

    struct InsuredItem
    {
        uint archId = 0;
        uint count = 0;
        std::string hardpoint = "";
    };

    struct Insurance
    {
        InsuranceType type = InsuranceType::None;
        int depositedMoney = 0;
        std::vector<InsuredItem> items;
    };

    std::unordered_map<std::string, Insurance> insuranceByCharacterFileName;

    static void ReadInsurance(const uint clientId)
    {
        const auto& filePath = GetInsuranceFilePath(clientId);
        try
        {
            if (std::filesystem::exists(filePath))
            {
                INI_Reader ini;
                if (ini.open(filePath.c_str(), false))
                {
                    Insurance& insurance = insuranceByCharacterFileName[GetCharacterFileName(clientId)];

                    while (ini.read_header())
                    {
                        if (ini.is_header("Insurance"))
                        {
                            while (ini.read_value())
                            {
                                if (ini.is_value("deposit"))
                                {
                                    insurance.depositedMoney = ini.get_value_int(0);
                                }
                                else if (ini.is_value("type"))
                                {
                                    const auto& readInsuranceType = ToLower(ini.get_value_string(0));
                                    InsuranceType insuranceType = InsuranceType::None;
                                    if (readInsuranceType == "equipment")
                                        insuranceType = InsuranceType::Equipment;
                                    else if (readInsuranceType == "consumables")
                                        insuranceType = InsuranceType::Consumables;
                                    else if (readInsuranceType == "all")
                                        insuranceType = InsuranceType::All;
                                    insurance.type = insuranceType;
                                }
                            }
                        }
                        else if (ini.is_header("Items"))
                        {
                            while (ini.read_value())
                            {
                                if (ini.is_value("item") && ini.get_num_parameters() == 3)
                                {
                                    InsuredItem item;
                                    item.archId = ini.get_value_int(0);
                                    item.count = ini.get_value_int(1);
                                    item.hardpoint = ini.get_value_string(2);
                                    insurance.items.push_back(item);
                                }
                            }
                        }
                    }
                    ini.close();
                }
            }
        }
        catch (std::filesystem::filesystem_error const& exception)
        {
            AddLog("Exception reading insurance file " + filePath);
        }
    }

    static void PersistInsuranceType(const uint clientId, const InsuranceType insuranceType)
    {
        insuranceByCharacterFileName[GetCharacterFileName(clientId)].type = insuranceType;

        const auto& filePath = GetInsuranceFilePath(clientId);
        std::string typeString;
        switch (insuranceType)
        {
            case InsuranceType::None:
                typeString = "none";
                break;

            case InsuranceType::All:
                typeString = "all";
                break;

            case InsuranceType::Equipment:
                typeString = "equipment";
                break;

            case InsuranceType::Consumables:
                typeString = "consumables";
                break;
        }
        std::filesystem::create_directory(outputDirectory);
        IniWrite(filePath, "Insurance", "type", typeString.c_str());
    }

    static void PersistInsuranceContents(const uint clientId, const int depositedMoney, const std::vector<InsuredItem>& entries)
    {
        Insurance& insurance = insuranceByCharacterFileName[GetCharacterFileName(clientId)];
        insurance.depositedMoney = depositedMoney;
        insurance.items = entries;

        const auto& filePath = GetInsuranceFilePath(clientId);
        std::filesystem::create_directory(outputDirectory);
        IniWrite(filePath, "Insurance", "deposit", std::to_string(depositedMoney));
        const size_t bufferSize = 65535;
        char* keyValues = static_cast<char*>(malloc(bufferSize));
        size_t bytesWritten = 0;
        keyValues[bytesWritten] = '\0';
        for (const auto& equip : entries)
        {
            const std::string str = "item = " + std::to_string(equip.archId) + ", " + std::to_string(equip.count) + ", " + equip.hardpoint;
            const size_t strLength = str.length() + 1; // +1 for \0
            if (bytesWritten + strLength > bufferSize)
                break;
            std::strncpy(&keyValues[bytesWritten], str.c_str(), strLength);
            bytesWritten += strLength;
        }
        if (bytesWritten < bufferSize)
            keyValues[bytesWritten] = '\0';
        WritePrivateProfileSectionA("Items", keyValues, filePath.c_str());
        free(keyValues);
    }

    static void DeleteInsuranceIfExisting(const std::string characterFileName)
    {
        const auto& filePath = outputDirectory + characterFileName + ".ini";
        try
        {
            std::filesystem::remove(filePath);
        }
        catch (std::filesystem::filesystem_error const& exception)
        {
            AddLog("Exception removing insurance file " + filePath);
        }
    }

    static void EnableInsuranceIfNotGiven(const uint clientId)
    {
        const auto& filePath = GetInsuranceFilePath(clientId);
        try
        {
            if (!std::filesystem::exists(filePath))
            {
                PersistInsuranceType(clientId, InsuranceType::All);
                PrintUserCmdText(clientId, L"Insurance was automatically fully activated. To deactivate, type: /insurance none");
            }
        }
        catch (std::filesystem::filesystem_error const& exception)
        {
            AddLog("Exception checking existence for insurance file " + filePath);
        }
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

    static std::wstring PrintMoney(const int64_t amount)
    {
        std::wstring result = std::to_wstring(amount);
        for (int pos = result.size() - 3; pos > 0; pos = pos - 3)
            result = result.insert(pos, L",");
        return L"$" + result;
    }

    static void CreateInsurance(const uint clientId)
    {
        const Insurance& insurance = insuranceByCharacterFileName[GetCharacterFileName(clientId)];
        std::vector<InsuredItem> insuredItems;
        int totalCost = 0;
        if (insurance.type != InsuranceType::None)
            totalCost = GetMaxShipHullAndCollGroupsRepairCost(clientId);

        for (const auto& equip : Players[clientId].equipDescList.equip)
        {
            if (equip.bMission)
                continue;

            const Archetype::Equipment* archetype = Archetype::GetEquipment(equip.get_arch_id());
            const GoodInfo* good = GoodList::find_by_id(equip.get_arch_id());
            if (!archetype || !good)
                continue;

            const Archetype::AClassType archetypeType = archetype->get_class_type();
            // Equip with price=0 is fixed equipment and must always be restored
            if (IsEquipment(archetypeType) && (good->fPrice == 0.0f || (insurance.type & InsuranceType::Equipment)))
            {
                totalCost += static_cast<int>(std::floor(good->fPrice));
                InsuredItem insured;
                insured.archId = equip.get_arch_id();
                insured.count = 1;
                insured.hardpoint = equip.get_hardpoint().value;
                insuredItems.push_back(insured);
            }
            else if (IsConsumable(archetypeType) && (insurance.type & InsuranceType::Consumables))
            {
                totalCost += static_cast<int>(std::floor(good->fPrice)) * equip.get_count();
                InsuredItem insured;
                insured.archId = equip.get_arch_id();
                insured.count = equip.get_count();
                insured.hardpoint = "";
                insuredItems.push_back(insured);
            }
        }

        // This checks either underflows or simply nothing to insure at all.
        if (totalCost < 0)
            return;

        int playerCash = 0;
        if (HK_ERROR error = HkGetCash(ARG_CLIENTID(clientId), playerCash); error != HKE_OK)
        {
            PrintUserCmdText(clientId, L"ERR Get cash failed err=" + HkErrGetText(error));
            return;
        }
        if (totalCost > playerCash)
        {
            PrintUserCmdText(clientId, L"Insuring ship failed. You need at least " + PrintMoney(totalCost) + L".");
            return;
        }

        if (HK_ERROR error = HkAddCash(ARG_CLIENTID(clientId), -totalCost); error != HKE_OK)
        {
            PrintUserCmdText(clientId, L"ERR Add cash failed err=" + HkErrGetText(error));
            return;
        }

        PersistInsuranceContents(clientId, totalCost, insuredItems);

        if (insurance.type != InsuranceType::None)
            PrintUserCmdText(clientId, L"Ship is insured for " + PrintMoney(totalCost) + L".");
    }

    static int RepairShipHullForMoney(const uint clientId, const int availableMoney)
    {
        const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
        const float shipMaxHitpoints = ship->fHitPoints;
        const int repairCost = static_cast<int>(std::round((1.0f - Players[clientId].fRelativeHealth) * shipMaxHitpoints * shipRepairCostFactor));

        if (repairCost > availableMoney)
            return 0;

        Server.ReqHullStatus(1.0f, clientId);
        return repairCost;
    }

    static int RepairCollisionGroupsForMoney(const uint clientId, const int availableMoney)
    {
        int repairCost = 0;
        const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
        const float shipMaxHitpoints = ship->fHitPoints;
        st6::list<CollisionGroupDesc, st6::allocator<CollisionGroupDesc> > repairedCollGroups;
        for (const auto& collGroup : Players[clientId].collisionGroupDesc)
        {
            Archetype::CollisionGroup* archCollGroup = ship->collisiongroup;
            // Find archetype of current collision group
            while (archCollGroup)
            {
                if (collGroup.id == archCollGroup->id)
                    break;
                archCollGroup = archCollGroup->next;
            }

            // Calculate repair cost and repair it
            if (archCollGroup)
            {
                const float collisionGroupMaxHitpoints = archCollGroup->hitPts;
                repairCost += static_cast<int>(std::round((1.0f - collGroup.health) * collisionGroupMaxHitpoints * shipRepairCostFactor));
            }

            CollisionGroupDesc repairedGroup;
            repairedGroup.id = collGroup.id;
            repairedGroup.health = 1.0f;
            repairedCollGroups.push_back(repairedGroup);
        }

        if (repairCost > availableMoney)
            return 0;

        Server.ReqCollisionGroups(repairedCollGroups, clientId);
        return repairCost;
    }

    static int RepairEquipmentForMoney(const uint clientId, const int availableMoney)
    {
        int repairCost = 0;
        EquipDescList repairedEquip;
        for (EquipDesc equip : Players[clientId].equipDescList.equip)
        {
            const Archetype::Equipment* equipment = Archetype::GetEquipment(equip.iArchID);
            if (equipment != nullptr && IsEquipment(equipment->get_class_type()))
            {
                const auto& good = GoodList::find_by_id(equip.iArchID);
                repairCost += static_cast<int>(std::round((1 - equip.fHealth) * good->fPrice * equipmentRepairCostFactor));
                equip.fHealth = 1.0f;
            }
            repairedEquip.equip.push_back(equip);
        }

        if (repairCost > availableMoney)
            return 0;

        // Repair all present equipment
        Players[clientId].equipDescList.equip.clear();
        Players[clientId].equipDescList.append(repairedEquip);
        // Set shadow list to prevent anti cheat detection
        Players[clientId].lShadowEquipDescList.equip.clear();
        Players[clientId].lShadowEquipDescList.append(repairedEquip);
        Server.ReqEquipment(repairedEquip, clientId);
        return repairCost;
    }

    struct RestoreOutput
    {
        int cost = 0;
        bool notEnoughCargoHold = false;
    };

    static RestoreOutput RestoreInsuredItems(const uint clientId, const int availableMoney)
    {
        const Insurance& insurance = insuranceByCharacterFileName[GetCharacterFileName(clientId)];
        if (insurance.items.empty())
        {
            RestoreOutput result;
            result.cost = 0;
            result.notEnoughCargoHold = false;
            return result;
        }

        const EquipDescList& playerEquipList = Players[clientId].equipDescList;
        const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
        float remainingCargoSpace = ship->fHoldSize - Players[clientId].equipDescList.get_cargo_space_occupied();
        const auto maxNanobots = ship->iMaxNanobots;
        const auto maxShieldBatteries = ship->iMaxShieldBats;
        std::vector<InsuredItem> itemToAdds;
        bool notEnoughCargoHold = false;
        float restoreCost = 0.0f;
        // Process insured equipment
        for (const auto& insuredItem : insurance.items)
        {
            const Archetype::Equipment* equipment = Archetype::GetEquipment(insuredItem.archId);
            if (equipment == nullptr)
                continue;

            if (IsEquipment(equipment->get_class_type()))
            {
                bool hardpointOccupied = false;
                for (const auto& presentEquip : playerEquipList.equip)
                {
                    if (std::string(presentEquip.szHardPoint.value) == insuredItem.hardpoint)
                    {
                        const Archetype::Equipment* foundEquipment = Archetype::GetEquipment(presentEquip.iArchID);
                        if (foundEquipment == nullptr)
                            continue;
                        hardpointOccupied = IsEquipment(equipment->get_class_type());
                        break;
                    }
                }
                if (!hardpointOccupied)
                {
                    // Create new item and add to cost list
                    itemToAdds.push_back(insuredItem);
                    const GoodInfo* good = GoodList::find_by_id(insuredItem.archId);
                    if (good)
                        restoreCost += good->fPrice;
                }
            }
            else if (IsConsumable(equipment->get_class_type()))
            {
                st6::list<EquipDesc>::const_iterator presentEquip;
                for (presentEquip = playerEquipList.equip.begin(); presentEquip != playerEquipList.equip.end(); presentEquip++)
                {
                    if (presentEquip->iArchID == insuredItem.archId)
                        break;
                }
                if (presentEquip == playerEquipList.equip.end())
                {
                    uint count = insuredItem.count;
                    if (equipment->get_class_type() == Archetype::AClassType::REPAIR_KIT)
                    {
                        count = min(count, maxNanobots);
                    }
                    else if (equipment->get_class_type() == Archetype::AClassType::SHIELD_BATTERY)
                    {
                        count = min(count, maxShieldBatteries);
                    }

                    const float requiredCargoSpace = equipment->fVolume * count;
                    if (remainingCargoSpace - requiredCargoSpace < 0.0f)
                    {
                        notEnoughCargoHold = true;
                        count = static_cast<uint>(std::floor(remainingCargoSpace / equipment->fVolume));
                    }
                    remainingCargoSpace -= equipment->fVolume * count;

                    // Create new equipment and add to cost list
                    if (count > 0)
                    {
                        itemToAdds.push_back(insuredItem);
                        itemToAdds.back().count = count;
                        const GoodInfo* good = GoodList::find_by_id(insuredItem.archId);
                        if (good)
                            restoreCost += good->fPrice * count;
                    }
                }
                else
                {
                    int countDiff = insuredItem.count - presentEquip->iCount;
                    if (countDiff <= 0)
                        continue;

                    if (equipment->get_class_type() == Archetype::AClassType::REPAIR_KIT)
                    {
                        countDiff = min(presentEquip->iCount + countDiff, maxNanobots - presentEquip->iCount);
                    }
                    else if (equipment->get_class_type() == Archetype::AClassType::SHIELD_BATTERY)
                    {
                        countDiff = min(presentEquip->iCount + countDiff, maxShieldBatteries - presentEquip->iCount);
                    }
                    if (countDiff <= 0)
                        continue;

                    const float requiredCargoSpace = equipment->fVolume * countDiff;
                    if (remainingCargoSpace - requiredCargoSpace < 0.0f)
                    {
                        notEnoughCargoHold = true;
                        countDiff = static_cast<int>(std::floor(remainingCargoSpace / equipment->fVolume));
                    }
                    remainingCargoSpace -= equipment->fVolume * countDiff;

                    // Fill up and add to cost list
                    if (countDiff > 0)
                    {
                        itemToAdds.push_back(insuredItem);
                        itemToAdds.back().count = countDiff;
                        const GoodInfo* good = GoodList::find_by_id(insuredItem.archId);
                        if (good)
                            restoreCost += good->fPrice * countDiff;
                    }
                }
            }
        }

        const int flooredRestoreCost = static_cast<int>(restoreCost);
        if (flooredRestoreCost > availableMoney)
        {
            RestoreOutput result;
            result.cost = 0;
            result.notEnoughCargoHold = false;
            return result;
        }

        const std::wstring clientWS = ARG_CLIENTID(clientId);
        // Add all items as cargo or equipment
        for (const auto& equipToRestore : itemToAdds)
        {
            if (!equipToRestore.hardpoint.empty())
                HkAddEquip(clientWS, equipToRestore.archId, equipToRestore.hardpoint, equipToRestore.count, true);
            else
                HkAddCargo(clientWS, equipToRestore.archId, equipToRestore.count, false);
        }

        RestoreOutput result;
        result.cost = flooredRestoreCost;
        result.notEnoughCargoHold = notEnoughCargoHold;
        return result;
    }

    static void ApplyInsurance(const int clientId)
    {
        const Insurance& insurance = insuranceByCharacterFileName[GetCharacterFileName(clientId)];
        if (insurance.depositedMoney <= 0 && insurance.items.empty())
            return;

        int availableMoney = insurance.depositedMoney;
        if (insurance.type != InsuranceType::None)
        {
            availableMoney -= RepairShipHullForMoney(clientId, availableMoney);
            availableMoney -= RepairCollisionGroupsForMoney(clientId, availableMoney);
        }
        if (insurance.type & InsuranceType::Equipment)
        {
            availableMoney -= RepairEquipmentForMoney(clientId, availableMoney);
        }
        const auto& result = RestoreInsuredItems(clientId, availableMoney);
        std::wstring resultMessage = L"Insured items restored.";
        if (result.notEnoughCargoHold)
            resultMessage += L" Some consumables could not be restored due to full cargo hold.";

        availableMoney -= result.cost;
        if (availableMoney > 0)
        {
            if (HK_ERROR error = HkAddCash(ARG_CLIENTID(clientId), availableMoney); error == HKE_OK)
                resultMessage += L" Unspent insurance deposit transferred back: " + PrintMoney(availableMoney);
            else
                PrintUserCmdText(clientId, L"ERR Add cash failed err=" + HkErrGetText(error));
        }
        PrintUserCmdText(clientId, resultMessage);
        PersistInsuranceContents(clientId, 0, std::vector<InsuredItem>({}));
    }

    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId)
    {
        std::wstring characterFileName;
        if (HkGetCharFileName(info.wszCharname, characterFileName) == HKE_OK)
            DeleteInsuranceIfExisting(wstos(characterFileName));
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId)
    {
        const std::string characterFileName = std::string(characterId.charFilename).substr(0, 11);
        DeleteInsuranceIfExisting(characterFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    std::unordered_set<uint> clientUndockedFromBase;

    void __stdcall DisConnect_After(unsigned int clientId, enum EFLConnection p2)
    {
        clientUndockedFromBase.erase(clientId);

        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall CharacterSelect_After(const CHARACTER_ID& cId, unsigned int clientId)
    {
        clientUndockedFromBase.erase(clientId);
        ReadInsurance(clientId);

        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId)
    {
        EnableInsuranceIfNotGiven(clientId);
        ApplyInsurance(clientId);

        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall BaseExit_After(unsigned int baseId, unsigned int clientId)
    {
        clientUndockedFromBase.insert(clientId);

        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
    {
        if (clientUndockedFromBase.contains(clientId))
        {
            const Insurance& insurance = insuranceByCharacterFileName[GetCharacterFileName(clientId)];
            CreateInsurance(clientId);
        }
        clientUndockedFromBase.erase(clientId);

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
        if (!oldCharacterFileName.empty())
        {
            const std::string sOldCharacterFileName = wstos(oldCharacterFileName);
            if (onlyDelete)
            {
                try
                {
                    std::filesystem::remove(outputDirectory + sOldCharacterFileName + ".ini");
                    insuranceByCharacterFileName.erase(sOldCharacterFileName);
                }
                catch (std::filesystem::filesystem_error const& exception)
                {
                    AddLog("Exception removing insurance file " + outputDirectory + sOldCharacterFileName + ".ini");
                }
            }
            else if (std::wstring newCharacterFileName; HkGetCharFileName(newCharname, newCharacterFileName) == HKE_OK)
            {
                const std::string sNewCharacterFileName = wstos(newCharacterFileName);
                try
                {
                    std::filesystem::rename(outputDirectory + sOldCharacterFileName + ".ini", outputDirectory + sNewCharacterFileName + ".ini");
                    insuranceByCharacterFileName[sNewCharacterFileName] = insuranceByCharacterFileName[sOldCharacterFileName];
                    insuranceByCharacterFileName.erase(sOldCharacterFileName);
                }
                catch (std::filesystem::filesystem_error const& exception)
                {
                    AddLog("Exception renaming insurance file " + outputDirectory + sOldCharacterFileName + ".ini to " + outputDirectory + sNewCharacterFileName + ".ini");
                }
            }
        }

        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }

    bool UserCmds(const uint clientId, const std::wstring& argumentsWS)
    {
        if (ToLower(argumentsWS).find(L"/insurance") == 0)
        {
            const std::wstring& arguments = ToLower(Trim(GetParamToEnd(argumentsWS, ' ', 1)));

            uint baseId = 0;
            pub::Player::GetBase(clientId, baseId);
            if (baseId)
            {
                if (arguments == L"none")
                {
                    PersistInsuranceType(clientId, InsuranceType::None);
                    PrintUserCmdText(clientId, L"Insurance deactivated.");
                }
                else if (arguments == L"equipment")
                {
                    PersistInsuranceType(clientId, InsuranceType::Equipment);
                    PrintUserCmdText(clientId, L"Insurance activated for ship and equipment.");
                }
                else if (arguments == L"consumables")
                {
                    PersistInsuranceType(clientId, InsuranceType::Consumables);
                    PrintUserCmdText(clientId, L"Insurance activated for ship and consumables.");
                }
                else if (arguments == L"all")
                {
                    PersistInsuranceType(clientId, InsuranceType::All);
                    PrintUserCmdText(clientId, L"Insurance activated for ship, equipment and consumables.");
                }
                else if (!arguments.empty())
                {
                    PrintUserCmdText(clientId, L"Available insurance options: none, equipment, consumables, all");
                }
            }

            if (arguments.empty())
            {
                switch (insuranceByCharacterFileName[GetCharacterFileName(clientId)].type)
                {
                case InsuranceType::None:
                    PrintUserCmdText(clientId, L"Insurance deactivated.");
                    break;

                case InsuranceType::Equipment:
                    PrintUserCmdText(clientId, L"Insurance activated for ship and equipment.");
                    break;

                case InsuranceType::Consumables:
                    PrintUserCmdText(clientId, L"Insurance activated for ship and consumables.");
                    break;

                case InsuranceType::All:
                    PrintUserCmdText(clientId, L"Insurance activated for ship, equipment and consumables.");
                    break;
                }
            }
            else if (!baseId)
            {
                PrintUserCmdText(clientId, L"Insurance can only be changed while docked to a base.");
            }

            returncode = SKIPPLUGINS_NOFUNCTIONCALL;
            return true;
        }
        returncode = DEFAULT_RETURNCODE;
        return false;
    }
}
