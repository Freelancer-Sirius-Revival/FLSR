#include "main.h"

namespace Insurance
{
    float insuranceEquipmentCostFactor;

    struct InsuredCargoItem
    {
        CARGO_INFO cargoInfo;
        GoodInfo goodInfo;
    };

    struct RestoreCargoItem
    {
        CARGO_INFO cargoInfo;
        float price;
    };

    enum class InsuranceType
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

    const std::unordered_map<InsuranceType, std::string> insuranceTypesStrings = {
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

    static std::string outputDirectory;
    static float shipRepairCostFactor = 0.33f;
    static float equipmentRepairCostFactor = 0.3f;

    static bool initialized = false;
    void Initialize()
    {
        if (initialized)
            return;
        initialized = true;

        const HMODULE commonHandle = GetModuleHandle("common.dll");
        if (commonHandle)
            shipRepairCostFactor = *(float*)(DWORD(commonHandle) + 0x004A28);

        const HMODULE freelancerHandle = GetModuleHandle("freelancer.exe");
        if (freelancerHandle)
            equipmentRepairCostFactor = *(float*)(DWORD(freelancerHandle) + 0x1D51B4);

        // Set up the directory name for all save files of this plugin.
        outputDirectory = scAcctPath + "\\insurances\\";
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


    static std::vector<std::string>& split(const std::string& str, char delim)
    {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string item;
        while (getline(ss, item, delim))
            result.push_back(item);
        return result;
    }

    static std::unordered_set<InsuranceType> ReadEnabledInsuranceTypes(const uint clientId)
    {
        INI_Reader ini;
        if (ini.open(GetInsuranceFilePath(clientId).c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("General"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("insured"))
                        {
                            const std::string result = ini.get_value_ptr();

                            while (result.find(','))

                            break;
                        }
                    }
                }
            }
            ini.close();
        }
        else
            return;


        const std::string hookUserFilePath = GetHookUserFilePath(clientId);

        std::wstring characterFileNameWS;
        if (hookUserFilePath.empty() || HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return std::set<InsuranceType>();
        const std::string section = INSURANCE_PREFIX_INI_SECTION + wstos(characterFileNameWS);

        std::set<InsuranceType> currentInsuranceTypes;
        for (const auto& insuranceTypeString : insuranceTypesStrings)
        {
            if (IniGetS(hookUserFilePath, section, insuranceTypeString.second, INSURANCE_OFF_VALUE) == INSURANCE_ON_VALUE)
                currentInsuranceTypes.insert(insuranceTypeString.first);
        }

        return currentInsuranceTypes;
    }

    static bool IsArchetypeTypeEquipment(const Archetype::AClassType archetypeType)
    {
        return archetypeType == Archetype::SHIELD_GENERATOR ||
               archetypeType == Archetype::THRUSTER ||
               archetypeType == Archetype::LAUNCHER ||
               archetypeType == Archetype::GUN ||
               archetypeType == Archetype::MINE_DROPPER ||
               archetypeType == Archetype::COUNTER_MEASURE_DROPPER;
    }

    static bool IsArchetypeTypeConsumable(const Archetype::AClassType archetypeType)
    {
        return archetypeType == Archetype::MINE ||
               archetypeType == Archetype::MUNITION ||
               archetypeType == Archetype::COUNTER_MEASURE ||
               archetypeType == Archetype::SHIELD_BATTERY ||
               archetypeType == Archetype::REPAIR_KIT;
    }

    static bool IsEquipmentInsuranceActive(const uint clientId)
    {
        const auto& insuredTypes = ReadEnabledInsuranceTypes(clientId);
        return insuredTypes.contains(InsuranceType::Equipment);
    }

    static bool IsConsumableInsuranceActive(const Archetype::AClassType archetypeType, const uint clientId)
    {
        const auto& insuredTypes = ReadEnabledInsuranceTypes(clientId);
        return  insuredTypes.contains(InsuranceType::Mines) && archetypeType == Archetype::MINE ||
                insuredTypes.contains(InsuranceType::Projectiles) && archetypeType == Archetype::MUNITION ||
                insuredTypes.contains(InsuranceType::Countermeasures) && archetypeType == Archetype::COUNTER_MEASURE ||
                insuredTypes.contains(InsuranceType::ShieldBatteries) && archetypeType == Archetype::SHIELD_BATTERY ||
                insuredTypes.contains(InsuranceType::Nanobots) && archetypeType == Archetype::REPAIR_KIT;
    }

    struct InsuredEquip
    {
        uint archId = 0;
        uint count = 0;
        std::string hardpoint = "";
    };

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

    static void CreateFixedEquipInsurance(const uint clientId)
    {
        const auto& filePath = GetInsuranceFilePath(clientId);
        for (const auto& equip : Players[clientId].equipDescList.equip)
        {
            if (!equip.bMission)
                continue;

            const Archetype::Equipment* archetype = Archetype::GetEquipment(equip.get_arch_id());
            const GoodInfo* good = GoodList::find_by_id(equip.get_arch_id());
            if (!archetype || !good)
                continue;

            const Archetype::AClassType archetypeType = archetype->get_class_type();

            // Equip with price=0 is fixed equipment and must always be restored
            if (IsArchetypeTypeEquipment(archetypeType) && good->fPrice == 0.0f)
                IniWrite(filePath, "Insured", "equipment", std::to_string(equip.get_arch_id()) + ", 1, " + equip.get_hardpoint().value);
        }
    }

    static void CreateFullInsurance(const uint clientId)
    {
        std::vector<InsuredEquip> insuredEquip;
        int totalCost = GetMaxShipHullAndCollGroupsRepairCost(clientId);
        for (const auto& equip : Players[clientId].equipDescList.equip)
        {
            if (!equip.bMission)
                continue;

            const Archetype::Equipment* archetype = Archetype::GetEquipment(equip.get_arch_id());
            const GoodInfo* good = GoodList::find_by_id(equip.get_arch_id());
            if (!archetype || !good)
                continue;

            const Archetype::AClassType archetypeType = archetype->get_class_type();

            // Equip with price=0 is fixed equipment and must always be restored
            if (IsArchetypeTypeEquipment(archetypeType) && (good->fPrice == 0.0f || IsEquipmentInsuranceActive(clientId)))
            {
                totalCost += static_cast<int>(std::floor(good->fPrice));
                InsuredEquip insured;
                insured.archId = equip.get_arch_id();
                insured.count = 1;
                insured.hardpoint = equip.get_hardpoint().value;
                insuredEquip.push_back(insured);
            }
            else if (IsArchetypeTypeConsumable(archetypeType) && IsConsumableInsuranceActive(archetypeType, clientId))
            {
                totalCost += static_cast<int>(std::floor(good->fPrice)) * equip.get_count();
                InsuredEquip insured;
                insured.archId = equip.get_arch_id();
                insured.count = equip.get_count();
                insured.hardpoint = equip.get_hardpoint().value;
                insuredEquip.push_back(insured);
            }
        }

        int playerCash = 0;
        if (HK_ERROR error = HkGetCash(ARG_CLIENTID(clientId), playerCash); error != HKE_OK)
        {
            PrintUserCmdText(clientId, L"ERR Get cash failed err=" + HkErrGetText(error));
            return;
        }
        if (totalCost > playerCash)
        {
            PrintUserCmdText(clientId, L"Insurance failed. You need at least " + PrintMoney(totalCost) + L".");
            return;
        }

        const auto& filePath = GetInsuranceFilePath(clientId);
        for (const auto& equip : insuredEquip)
            IniWrite(filePath, "Insured", "equipment", std::to_string(equip.archId) + ", " + std::to_string(equip.count) + ", " + equip.hardpoint);

        PrintUserCmdText(clientId, L"Insurance deposited " + PrintMoney(totalCost) + L" until next dock at a base.");
    }

    static int RepairShipHull(const uint clientId)
    {
        const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
        const float shipMaxHitpoints = ship->fHitPoints;
        const int repairCost = static_cast<int>(std::round((1.0f - Players[clientId].fRelativeHealth) * shipMaxHitpoints * shipRepairCostFactor));
        Server.ReqHullStatus(1.0f, clientId);
        return repairCost;
    }

    static int RepairCollisionGroups(const uint clientId)
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
        Server.ReqCollisionGroups(repairedCollGroups, clientId);
        return repairCost;
    }

    static int RepairEquipment(const uint clientId)
    {
        int repairCost = 0;
        EquipDescList repairedEquip;
        for (EquipDesc equip : Players[clientId].equipDescList.equip)
        {
            if (equip.bMounted) // Commodities in cargo pods also can count as mounted
            {
                const Archetype::Equipment* equipment = Archetype::GetEquipment(equip.iArchID);
                if (equipment && IsArchetypeTypeEquipment(equipment->get_class_type()))
                {
                    const auto& good = GoodList::find_by_id(equip.iArchID);
                    repairCost += static_cast<int>(std::round((1 - equip.fHealth) * good->fPrice * equipmentRepairCostFactor));
                    equip.fHealth = 1.0f;
                }
            }
            repairedEquip.equip.push_back(equip);
        }

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

    static RestoreOutput RestoreInsuredEquipment(const uint clientId)
    {
        // Get all insured equipment
        std::vector<InsuredEquip> insuredEquipment;
        const std::string insuranceFilePath = GetInsuranceFilePath(clientId);
        INI_Reader ini;
        if (ini.open(insuranceFilePath.c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("Insured"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("equipment"))
                        {
                            InsuredEquip equip;
                            equip.archId = ini.get_value_int(0);
                            equip.count = ini.get_value_int(1);
                            equip.hardpoint = ini.get_value_string(2);
                            insuredEquipment.push_back(equip);
                        }
                    }
                }
            }
            ini.close();
        }
        
        if (insuredEquipment.empty())
            return;

        const EquipDescList& playerEquipList = Players[clientId].equipDescList;
        const Archetype::Ship* ship = Archetype::GetShip(Players[clientId].iShipArchetype);
        float remainingCargoSpace = ship->fHoldSize - Players[clientId].equipDescList.get_cargo_space_occupied();
        std::vector<InsuredEquip> equipToAdd;
        bool notEnoughCargoHold = false;
        float restoreCost = 0.0f;
        // Process insured equipment
        for (const auto& insuredEquip : insuredEquipment)
        {
            // Mounted Equipment
            if (!insuredEquip.hardpoint.empty())
            {
                // Check if the hardpoint is still in use on the ship
                bool alreadyPresent = false;
                for (const auto& presentEquip : playerEquipList.equip)
                {
                    alreadyPresent = presentEquip.bMounted && insuredEquip.hardpoint.compare(presentEquip.szHardPoint.value) == 0;
                    if (alreadyPresent)
                    {
                        if (insuredEquip.archId != presentEquip.iArchID)
                            PrintUserCmdText(clientId, L"Report to Admin: Equip on hardpoint not matching: " + stows(insuredEquip.hardpoint) + L", " + std::to_wstring(insuredEquip.archId) + L" : " + std::to_wstring(presentEquip.iArchID));
                        break;
                    }
                }
                if (alreadyPresent)
                    continue;

                // Create new equipment and add to cost list
                equipToAdd.push_back(insuredEquip);
                const GoodInfo* good = GoodList::find_by_id(insuredEquip.archId);
                if (good)
                    restoreCost += good->fPrice;
            }
            else
            {
                st6::list<EquipDesc>::const_iterator presentEquip;
                for (presentEquip = playerEquipList.equip.begin(); presentEquip != playerEquipList.equip.end(); presentEquip++)
                {
                    if (insuredEquip.archId == presentEquip->iArchID)
                        break;
                }

                // The consumable exists already
                if (presentEquip != playerEquipList.equip.end())
                {
                    int countDiff = insuredEquip.count - presentEquip->iCount;
                    if (countDiff <= 0)
                        continue;

                    const Archetype::Equipment* equipment = Archetype::GetEquipment(insuredEquip.archId);
                    if (equipment)
                    {
                        if (equipment->get_class_type() == Archetype::AClassType::REPAIR_KIT)
                        {
                            countDiff = static_cast<int>(std::min(presentEquip->iCount + countDiff, ship->iMaxNanobots - presentEquip->iCount));
                        }
                        else if (equipment->get_class_type() == Archetype::AClassType::SHIELD_BATTERY)
                        {
                            countDiff = static_cast<int>(std::min(presentEquip->iCount + countDiff, ship->iMaxShieldBats - presentEquip->iCount));
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
                    }
                    else
                        continue;

                    // Fill up and add to cost list
                    if (countDiff > 0)
                    {
                        equipToAdd.push_back(insuredEquip);
                        equipToAdd.back().count = countDiff;
                        const GoodInfo* good = GoodList::find_by_id(insuredEquip.archId);
                        if (good)
                            restoreCost += good->fPrice * countDiff;
                    }
                }
                else
                {
                    int count = insuredEquip.count;
                    const Archetype::Equipment* equipment = Archetype::GetEquipment(insuredEquip.archId);
                    if (equipment)
                    {
                        if (equipment->get_class_type() == Archetype::AClassType::REPAIR_KIT)
                        {
                            count = std::min(count, static_cast<int>(ship->iMaxNanobots));
                        }
                        else if (equipment->get_class_type() == Archetype::AClassType::SHIELD_BATTERY)
                        {
                            count = std::min(count, static_cast<int>(ship->iMaxShieldBats));
                        }

                        const float requiredCargoSpace = equipment->fVolume * count;
                        if (remainingCargoSpace - requiredCargoSpace < 0.0f)
                        {
                            notEnoughCargoHold = true;
                            count = static_cast<int>(std::floor(remainingCargoSpace / equipment->fVolume));
                        }
                        remainingCargoSpace -= equipment->fVolume * count;
                    }
                    else
                        continue;

                    // Create new equipment and add to cost list
                    if (count > 0)
                    {
                        equipToAdd.push_back(insuredEquip);
                        equipToAdd.back().count = count;
                        const GoodInfo* good = GoodList::find_by_id(insuredEquip.archId);
                        if (good)
                            restoreCost += good->fPrice * count;
                    }
                }
            }
        }

        const std::wstring clientWS = ARG_CLIENTID(clientId);
        // Add all items as cargo or equipment
        for (const auto& equipToRestore : equipToAdd)
        {
            if (equipToRestore.hardpoint.empty())
                HkAddCargo(clientWS, equipToRestore.archId, equipToRestore.count, false);
            else
                HkAddEquip(clientWS, equipToRestore.archId, equipToRestore.hardpoint, true);
        }

        RestoreOutput result;
        result.cost = static_cast<int>(restoreCost);
        result.notEnoughCargoHold = notEnoughCargoHold;
        return result;
    }

    static void UseInsurance(const uint clientId)
    {
        // Get all insured equipment
        int pawn = 0;
        bool onlyFixedEquip = false;
        const std::string insuranceFilePath = GetInsuranceFilePath(clientId);
        INI_Reader ini;
        if (ini.open(insuranceFilePath.c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("General"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("only_fixed_equipment"))
                            onlyFixedEquip = ini.get_bool(0);
                        else if (ini.is_value("pawn"))
                            pawn = ini.get_value_int(0);
                    }
                }
            }
            ini.close();
        }
        else
            return;

        int shipRepairCost = 0;
        int equipmentRepairCost = 0;
        if (!onlyFixedEquip)
        {
            shipRepairCost = RepairShipHull(clientId);
            shipRepairCost += RepairCollisionGroups(clientId);
            equipmentRepairCost = RepairEquipment(clientId);
        }
        const auto& restoreEquipResult = RestoreInsuredEquipment(clientId);

        // Calculate total price and remove from bank account
        const int totalPrice = shipRepairCost + equipmentRepairCost + restoreEquipResult.cost;
        const int moneyBack = std::max(0, pawn - totalPrice);
        if (moneyBack > 0)
        {
            if (HK_ERROR error = HkAddCash(ARG_CLIENTID(clientId), moneyBack); error != HKE_OK)
            {
                PrintUserCmdText(clientId, L"ERR Add cash failed err=" + HkErrGetText(error));
            }
        }

        std::wstring restorationOutput = L"";
        if (shipRepairCost > 0 || equipmentRepairCost > 0)
            restorationOutput = L"Repairs done";
        if (restoreEquipResult.cost > 0)
        {
            if (!restorationOutput.empty())
                restorationOutput += L" and ";
            else
                restorationOutput = L"Your ";
            restorationOutput += L"insured items were restored";
        }
        if (!restorationOutput.empty())
            restorationOutput += L". ";
        if (restoreEquipResult.notEnoughCargoHold)
            restorationOutput += L"Some consumables could not be restored due to full cargo hold. ";
        if (moneyBack > 0)
            restorationOutput += L"Unspent $" + PrintMoney(moneyBack) + L" from insurance returned.";
        if (!restorationOutput.empty())
            PrintUserCmdText(clientId, restorationOutput);

        HkSaveChar(clientId);
        IniWrite(insuranceFilePath, "General", "pawn", "0");
        IniDelSection(insuranceFilePath, "Insured");
    }

    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        UseInsurance(clientId);
    }

    void __stdcall BaseExit(unsigned int baseId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (insuranceType == InsuranceType::None)
            CreateFixedEquipInsurance(clientId);
        else
            CreateFullInsurance(clientId);
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

    std::string GetCurrentlyInsuredTypesJoinedString(const uint clientId)
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

    void UserCMD_INSURANCE(const uint clientId, const std::wstring& argumentsWS)
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

            uint shipId;
            pub::Player::GetShip(clientId, shipId);
            if (shipId)
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
            uint shipId;
            pub::Player::GetShip(clientId, shipId);
            if (shipId)
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

    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId)
    {
        std::wstring characterFileName;
        if (HkGetCharFileName(info.wszCharname, characterFileName) != HKE_OK)
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }
        DeleteInsuranceFileIfExisting(characterFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId)
    {
        const std::wstring characterFileName = stows(std::string(characterId.charFilename).substr(0, 11));
        DeleteInsuranceFileIfExisting(characterFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    static std::wstring characterFileNameToRename;

    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        if (onlyDelete || HkGetCharFileName(charname, characterFileNameToRename) != HKE_OK)
            characterFileNameToRename = L"";
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }

    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        if (!characterFileNameToRename.empty())
        {
            std::wstring characterFileName;
            if (HkGetCharFileName(newCharname, characterFileName) == HKE_OK)
            {
                const std::string oldCharFilePath = GetInsuranceFilePath(characterFileNameToRename);
                const std::string newCharFilePath = GetInsuranceFilePath(characterFileName);
                CopyFile(oldCharFilePath.c_str(), newCharFilePath.c_str(), FALSE);
                DeleteFile(oldCharFilePath.c_str());
            }
        }
        characterFileNameToRename = L"";
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }
}
