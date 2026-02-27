#include "Carrier.h"
#include <algorithm>
#include "Cloak.h"
#include "Plugin.h"

/**
* CARRIER PLUGIN
*
* by Skotty
* 
* Players can dock to Carriers when:
    Player and Carrier are in same Group
    Carrier has slots free
    Player is no carrier
    Player is completely uncloaked
    Carrier is completely uncloaked

* Players cannot undock from Carrier when:
    Carrier is cloaked
    Carrier is in Tradelane
    Carrier is docking with jump object/in jump tunnel

* Players will always undock to:
    Carrier's current position in space when it is in space
    Carrier's base when it is docked/dead
    Last dock-in position of the player/carrier's last docked base (whichever happened  most recent) if carrier is not logged in/sold ship
    Player's/carrier last regular base if carrier (whichever happened  most recent) was left while being unrelated to any carrier (Fallback)

* Save last dock state with carriers

* Slots will be filled when:
    Player enters the Carrier

* Slots will be freed when:
    Carrier sells ship
    Player lands on a different base
    Player leaves carrier while Carrier is not logged in
    Player (forcefully gets) unassigned & leaves carrier
*/

namespace Carrier
{
    struct CarrierDefinition
    {
        uint shipArchetypeId = 0;
        uint slots = 0;
        std::wstring baseNickname = L"";
        uint baseId = 0;
        float dockOffset[3] = { 0,0,0 };
    };
    std::unordered_map<uint, CarrierDefinition> carrierDefinitionByShipArchetypeId;

    enum class DockState
    {
        Docked,
        Undocked,
        Undocking
    };

    struct CarrierAssignment
    {
        std::string characterFileName = "";
        std::string carrierFileName = "";
        uint carrierShiparchId = 0;
        bool assigned = false;
        DockState dockState = DockState::Undocked;
    };
    std::unordered_map<uint, CarrierAssignment> carrierAssignmentByCharacterFileNameId;
    std::unordered_map<uint, std::wstring> lastRegularBaseNameByCharacterFileNameId;

    struct Location
    {
        uint systemId = 0;
        Vector position;
        Matrix orientation;
    };
    std::unordered_map<uint, Location> dockLocationByCharacterFileNameIds;

    struct UndockPath
    {
        std::vector<uint> remainingJumpPath;
        Location lastDockLocation;
    };
    std::unordered_map<uint, UndockPath> undockPathByClientId;

    std::unordered_map<uint, std::vector<uint>> shortestSystemPathToTargets;
    // The order of systems to jump through is reversed to use pop_back() when traversing a path.
    std::unordered_map<uint, std::vector<uint>> shortestJumpObjectPathToTargets;

    uint dockSystemId = 0;
    float dockRadius = 100.0f;

    std::set<uint> carrierClientIdInJump;

    static std::string GetCarrierMapFilePath()
    {
        return scAcctPath + "\\carriers.ini";
    }

    static std::string GetCharacterFileName(const uint clientId)
    {
        if(!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return "";
        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return "";
        return wstos(characterFileNameWS);
    }

    /**
    * Assigning a character to a carrier means they will respawn from this carrier each time they die.
    */
    static void AssignCharacterToCarrier(const std::string& characterFileName, const std::string& carrierFileName, const uint carrierShiparchId)
    {
        const uint characterFileNameId = CreateID(characterFileName.c_str());
        carrierAssignmentByCharacterFileNameId[characterFileNameId].characterFileName = characterFileName;
        carrierAssignmentByCharacterFileNameId[characterFileNameId].carrierFileName = carrierFileName;
        carrierAssignmentByCharacterFileNameId[characterFileNameId].assigned = true;
        carrierAssignmentByCharacterFileNameId[characterFileNameId].carrierShiparchId = carrierShiparchId;
        const std::string carrierFilePath = GetCarrierMapFilePath();
        IniWrite(carrierFilePath, characterFileName, "assignment", carrierFileName + ", true");
        IniWrite(carrierFilePath, characterFileName, "carrier_shiparchetype", std::to_string(carrierShiparchId));
    }

    /**
    * Unassigning a character from a carrier means they will still only spawn from the carrier one last time.
    */
    static void UnassignCharacterFromCarrier(const std::string& characterFileName)
    {
        const uint characterFileNameId = CreateID(characterFileName.c_str());
        if (carrierAssignmentByCharacterFileNameId.contains(characterFileNameId))
        {
            carrierAssignmentByCharacterFileNameId[characterFileNameId].assigned = false;
            IniWrite(GetCarrierMapFilePath(), characterFileName, "assignment", carrierAssignmentByCharacterFileNameId[characterFileNameId].carrierFileName + ", false");
        }
    }

    static void DeleteCharacterCarrierDockLocation(const std::string& characterFileName)
    {
        dockLocationByCharacterFileNameIds.erase(CreateID(characterFileName.c_str()));
        IniDelete(GetCarrierMapFilePath(), characterFileName, "carrier_dock_location");
    }

    static void RemoveCharacterFromCarrier(const std::string characterFileName)
    {
        const std::string& carrierFilePath = GetCarrierMapFilePath();
        carrierAssignmentByCharacterFileNameId.erase(CreateID(characterFileName.c_str()));
        IniDelete(carrierFilePath, characterFileName, "assignment");
        IniDelete(carrierFilePath, characterFileName, "dock_state");
        IniDelete(carrierFilePath, characterFileName, "carrier_shiparchetype");
    }

    static bool IsLinkedToCarrier(const uint characterFileNameId)
    {
        return carrierAssignmentByCharacterFileNameId.contains(characterFileNameId) && !carrierAssignmentByCharacterFileNameId[characterFileNameId].carrierFileName.empty();
    }

    static bool IsCarrierBase(const uint baseId)
    {
        for (const auto& carrierDefinition : carrierDefinitionByShipArchetypeId)
        {
            if (carrierDefinition.second.baseId == baseId)
                return true;
        }
        return false;
    }

    static void SaveCharacterLastRegularBase(const std::string& characterFileName, const uint baseId)
    {
        if (IsCarrierBase(baseId))
            return;
        const std::wstring baseName = HkGetBaseNickByID(baseId);
        lastRegularBaseNameByCharacterFileNameId[CreateID(characterFileName.c_str())] = baseName;
        IniWrite(GetCarrierMapFilePath(), characterFileName, "last_regular_base", wstos(baseName));
    }

    static void SaveCharacterCarrierDockLocation(const std::string& characterFileName, const uint systemId, const Vector& position, const Matrix& orientation)
    {
        const uint characterFileNameId = CreateID(characterFileName.c_str());
        Location location;
        location.systemId = systemId;
        location.position = position;
        location.orientation = orientation;
        dockLocationByCharacterFileNameIds[characterFileNameId] = location;
        IniWrite(GetCarrierMapFilePath(), characterFileName, "carrier_dock_location", std::to_string(systemId) + ", " +
            std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ", " +
            std::to_string(orientation.data[0][0]) + ", " + std::to_string(orientation.data[0][1]) + ", " + std::to_string(orientation.data[0][2]) + ", " +
            std::to_string(orientation.data[1][0]) + ", " + std::to_string(orientation.data[1][1]) + ", " + std::to_string(orientation.data[1][2]) + ", " +
            std::to_string(orientation.data[2][0]) + ", " + std::to_string(orientation.data[2][1]) + ", " + std::to_string(orientation.data[2][2])
        );
    }

    static void SaveCharacterDockState(const std::string& characterFileName, const DockState dockState)
    {
        const uint characterFileNameId = CreateID(characterFileName.c_str());
        carrierAssignmentByCharacterFileNameId[characterFileNameId].dockState = dockState;
        std::string dockStateString;
        switch (dockState)
        {
            case DockState::Docked:
                dockStateString = "docked";
                break;
            case DockState::Undocked:
                dockStateString = "undocked";
                break;
            case DockState::Undocking:
                dockStateString = "undocking";
                break;
        }
        IniWrite(GetCarrierMapFilePath(), characterFileName, "dock_state", dockStateString);
    }

    static void ReadCharacterData()
    {
        INI_Reader ini;
        if (ini.open(GetCarrierMapFilePath().c_str(), false))
        {
            while (ini.read_header())
            {
                const std::string characterFileName = std::string(ini.get_header_ptr());
                const uint characterFileNameId = CreateID(characterFileName.c_str());
                while (ini.read_value())
                {
                    CarrierAssignment& assignment = carrierAssignmentByCharacterFileNameId[characterFileNameId];
                    if (ini.is_value("assignment"))
                    {
                        assignment.characterFileName = characterFileName;
                        assignment.carrierFileName = ini.get_value_string(0);
                        assignment.assigned = ini.get_bool(1);
                    }
                    if (ini.is_value("carrier_shiparchetype"))
                        assignment.carrierShiparchId = ini.get_value_int(0);
                    if (ini.is_value("dock_state"))
                    {
                        std::string dockState = ini.get_value_string(0);
                        std::transform(dockState.begin(), dockState.end(), dockState.begin(), ::tolower);
                        if (dockState.compare("undocked") == 0)
                            assignment.dockState = DockState::Undocked;
                        else if (dockState.compare("undocking") == 0)
                            assignment.dockState = DockState::Undocking;
                        else if (dockState.compare("docked") == 0)
                            assignment.dockState = DockState::Docked;
                    }
                    if (ini.is_value("last_regular_base"))
                        lastRegularBaseNameByCharacterFileNameId[characterFileNameId] = stows(std::string(ini.get_value_string(0)));
                    if (ini.is_value("carrier_dock_location"))
                    {
                        Location& location = dockLocationByCharacterFileNameIds[characterFileNameId];
                        location.systemId = ini.get_value_int(0);
                        location.position.x = ini.get_value_float(1);
                        location.position.y = ini.get_value_float(2);
                        location.position.z = ini.get_value_float(3);
                        location.orientation.data[0][0] = ini.get_value_float(4);
                        location.orientation.data[0][1] = ini.get_value_float(5);
                        location.orientation.data[0][2] = ini.get_value_float(6);
                        location.orientation.data[1][0] = ini.get_value_float(7);
                        location.orientation.data[1][1] = ini.get_value_float(8);
                        location.orientation.data[1][2] = ini.get_value_float(9);
                        location.orientation.data[2][0] = ini.get_value_float(10);
                        location.orientation.data[2][1] = ini.get_value_float(11);
                        location.orientation.data[2][2] = ini.get_value_float(12);
                    }
                }
            }
            ini.close();
        }
    }

    static void LoadSettings()
    {
        char currentDirectory[MAX_PATH];
        GetCurrentDirectory(sizeof(currentDirectory), currentDirectory);
        const std::string configFilePath = std::string(currentDirectory) + "\\flhook_plugins\\FLSR-Carrier.cfg";

        carrierDefinitionByShipArchetypeId.clear();
        INI_Reader ini;
        if (ini.open(configFilePath.c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("General"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("dock_system"))
                            dockSystemId = CreateID(ini.get_value_string(0));
                        if (ini.is_value("dock_radius"))
                            dockRadius = ini.get_value_float(0);
                    }
                }

                if (ini.is_header("Ship"))
                {
                    CarrierDefinition definition;
                    while (ini.read_value())
                    {
                        if (ini.is_value("ship_nickname"))
                            definition.shipArchetypeId = CreateID(ini.get_value_string(0));
                        else if (ini.is_value("slots"))
                            definition.slots = ini.get_value_int(0);
                        else if (ini.is_value("base_nickname"))
                        {
                            definition.baseNickname = stows(ini.get_value_string(0));
                            definition.baseId = CreateID(ini.get_value_string(0));
                        }
                        else if (ini.is_value("dock_offset"))
                        {
                            definition.dockOffset[0] = ini.get_value_float(0);
                            definition.dockOffset[1] = ini.get_value_float(1);
                            definition.dockOffset[2] = ini.get_value_float(2);
                        }
                    }
                    if (definition.shipArchetypeId && ((definition.slots > 0 && !definition.baseNickname.empty()) || definition.slots == 0))
                        carrierDefinitionByShipArchetypeId[definition.shipArchetypeId] = definition;
                }
            }
            ini.close();
        }

        if (!dockSystemId)
            return;

        std::string systemsShortestPathFilePath = "";
        if (ini.open("freelancer.ini", false))
        {
            while (ini.read_header() && systemsShortestPathFilePath.empty())
            {
                if (ini.is_header("Freelancer"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("data path"))
                        {
                            systemsShortestPathFilePath = ini.get_value_string(0);
                            systemsShortestPathFilePath += "\\UNIVERSE\\systems_shortest_path.ini";
                            break;
                        }
                    }
                }
            }
            ini.close();
        }

        if (!systemsShortestPathFilePath.empty() && ini.open(systemsShortestPathFilePath.c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("SystemConnections"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("Path") && CreateID(ini.get_value_string(0)) == dockSystemId)
                        {
                            const uint targetId = CreateID(ini.get_value_string(1));
                            if (!targetId)
                                continue;
                            uint pos = 2;
                            std::string value = ini.get_value_string(pos++);
                            while (!value.empty())
                            {
                                shortestSystemPathToTargets[targetId].push_back(CreateID(value.c_str()));
                                value = ini.get_value_string(pos++);
                            }
                        }
                    }
                }
            }
            ini.close();
        }
    }

    bool initialized = false;

    // This must be executed AFTER LoadSettings and when the game data has already been stored to memory.
    void InitializeWithGameData()
    {
        if (initialized)
            return;
        initialized = true;

        ConPrint(L"Initializing Carrier... ");

        LoadSettings();

        std::vector<CSolar*> jumpObjects;
        CSolar* solar = static_cast<CSolar*>(CObject::FindFirst(CObject::CSOLAR_OBJECT));
        while (solar != NULL)
        {
            if (solar->get_type() == ObjectType::JumpHole)
                jumpObjects.push_back(solar);
            solar = static_cast<CSolar*>(solar->FindNext());
        }

        for (const auto& systemPath : shortestSystemPathToTargets)
        {
            std::vector<uint> foundJumpObjectIds;
            for (size_t index = 1; index < systemPath.second.size(); index++)
            {
                for (const auto& jumpObject : jumpObjects)
                {
                    if (jumpObject->system == systemPath.second[index - 1] && jumpObject->get_dest_system() == systemPath.second[index])
                    {
                        foundJumpObjectIds.push_back(jumpObject->get_id());
                        break;
                    }
                }
            }
            // Only keep those entries which have the same amount of jump objects as system transitions are necessary (systemPath - 1). Otherwise it would be an incomplete path.
            if (foundJumpObjectIds.size() == systemPath.second.size() - 1)
            {
                std::reverse(foundJumpObjectIds.begin(), foundJumpObjectIds.end());
                shortestJumpObjectPathToTargets[systemPath.first] = foundJumpObjectIds;
            }
        }

        ReadCharacterData();

        ConPrint(L"Done\n");
    }

    static bool AreInSameGroup(const uint clientAId, const uint clientBId)
    {
        if (!HkIsValidClientID(clientAId) || !HkIsValidClientID(clientBId))
            return false;

        std::list<GROUP_MEMBER> members;
        if (HkGetGroupMembers(ARG_CLIENTID(clientAId), members) == HKE_OK)
        {
            for (const auto& member : members)
            {
                if (member.iClientID == clientBId)
                    return true;
            }
        }
        return false;
    }

    static bool IsCarrier(const uint clientId)
    {
        if (!HkIsValidClientID(clientId))
            return false;

        uint shiparchId;
        pub::Player::GetShipID(clientId, shiparchId);
        if (!shiparchId)
            return false;

        return carrierDefinitionByShipArchetypeId.contains(shiparchId) && carrierDefinitionByShipArchetypeId[shiparchId].slots > 0;
    }

    static void TryDock(const uint clientId)
    {
        const std::string& clientFileName = GetCharacterFileName(clientId);
        if (clientFileName.empty())
            return;

        uint systemId;
        pub::Player::GetSystem(clientId, systemId);
        if (!systemId)
            return;

        uint shipId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;

        uint targetId;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return;

        uint targetClientId = HkGetClientIDByShip(targetId);
        if (!targetClientId)
            return;

        uint targetShiparchId;
        pub::Player::GetShipID(targetClientId, targetShiparchId);
        if (!targetShiparchId)
            return;

        const std::string& targetFileName = GetCharacterFileName(targetClientId);
        if (targetFileName.empty())
            return;
        const uint targetFileNameId = CreateID(targetFileName.c_str());

        if (!IsCarrier(targetClientId))
            return;

        uint clientShiparchId;
        pub::Player::GetShipID(clientId, clientShiparchId);
        if (carrierDefinitionByShipArchetypeId.contains(clientShiparchId))
        {
            PrintUserCmdText(clientId, L"Your ship is too big for carriers!");
            pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cannot_dock"));
            return;
        }

        if (Cloak::GetClientCloakState(clientId) != Cloak::CloakState::Uncloaked)
        {
            PrintUserCmdText(clientId, L"Docking with activated cloak not possible!");
            pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cannot_dock"));
            return;
        }

        const auto& carrierDefinition = carrierDefinitionByShipArchetypeId[targetShiparchId];

        if (!AreInSameGroup(clientId, targetClientId))
        {
            PrintUserCmdText(clientId, L"The carrier must be in the same group as yours!");
            pub::Player::SendNNMessage(clientId, pub::GetNicknameId("dock_disallowed"));
            return;
        }

        if (Cloak::GetClientCloakState(targetClientId) != Cloak::CloakState::Uncloaked)
        {
            PrintUserCmdText(clientId, L"The carrier must be fully uncloaked to dock!");
            pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cannot_dock"));
            return;
        }

        //uint filledSlots = 0;
        //for (const auto& assignment : carrierAssignmentByCharacterFileNameId)
        //{
        //    if (!assignment.second.carrierFileName.empty() && CreateID(assignment.second.carrierFileName.c_str()) == targetFileNameId)
        //        filledSlots++;
        //}

        //if (filledSlots >= carrierDefinition.slots)
        //{
        //    PrintUserCmdText(clientId, L"The carrier is already full!");
        //    pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cannot_dock"));
        //    return;
        //}

        Vector shipVector;
        Matrix shipRotation;
        pub::SpaceObj::GetLocation(shipId, shipVector, shipRotation);
        Vector targetVector;
        Matrix targetRotation;
        pub::SpaceObj::GetLocation(targetId, targetVector, targetRotation);
        TranslateY(targetVector, targetRotation, carrierDefinition.dockOffset[0]);
        TranslateZ(targetVector, targetRotation, carrierDefinition.dockOffset[1]);
        TranslateX(targetVector, targetRotation, carrierDefinition.dockOffset[2]);
        if (HkDistance3D(shipVector, targetVector) > dockRadius)
        {
            PrintUserCmdText(clientId, L"Too far away from carrier's dock!");
            pub::Player::SendNNMessage(clientId, pub::GetNicknameId("nnv_dock_too_far"));
            return;
        }

        if (HkBeam(ARG_CLIENTID(clientId), carrierDefinition.baseNickname) == HKE_OK)
        {
            AssignCharacterToCarrier(clientFileName, targetFileName, carrierDefinition.shipArchetypeId);
            SaveCharacterCarrierDockLocation(clientFileName, systemId, shipVector, shipRotation);
        }
    }

    static uint FindCarrierClientIdBySlottedClientId(const uint slottedClientId)
    {
        const std::string& fileName = GetCharacterFileName(slottedClientId);
        if (fileName.empty())
            return 0;
        const uint fileNameId = CreateID(fileName.c_str());
        if (!IsLinkedToCarrier(fileNameId))
            return 0;

        PlayerData* playerData = 0;
        while (playerData = Players.traverse_active(playerData))
        {
            const uint clientId = HkGetClientIdFromPD(playerData);
            if (strcmp(carrierAssignmentByCharacterFileNameId[fileNameId].carrierFileName.c_str(), GetCharacterFileName(clientId).c_str()) == 0)
                return clientId;
        }
        return 0;
    }

    static bool IsCarrierReachable(const uint clientId, const uint carrierId)
    {
        if (!HkIsValidClientID(carrierId))
            return false;

        if (HkIsInCharSelectMenu(carrierId))
            return false;

        uint carrierShipId;
        pub::Player::GetShip(carrierId, carrierShipId);
        if (carrierShipId)
        {
            if (Cloak::GetClientCloakState(carrierId) != Cloak::CloakState::Uncloaked)
            {
                PrintUserCmdText(clientId, L"Carrier is cloaked. Cannot undock.");
                pub::Player::SendNNMessage(clientId, pub::GetNicknameId("launch_to_space"));
                pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
                return false;
            }

            if (carrierClientIdInJump.contains(carrierId))
            {
                PrintUserCmdText(clientId, L"Carrier is jumping. Cannot undock.");
                pub::Player::SendNNMessage(clientId, pub::GetNicknameId("launch_to_space"));
                pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
                return false;
            }

            const CShip* carrierShip = (CShip*)CObject::Find(carrierShipId, CObject::CSHIP_OBJECT);
            if (carrierShip && carrierShip->is_using_tradelane())
            {
                PrintUserCmdText(clientId, L"Carrier is in Trade Lane. Cannot undock.");
                pub::Player::SendNNMessage(clientId, pub::GetNicknameId("launch_to_space"));
                pub::Player::SendNNMessage(clientId, pub::GetNicknameId("cancelled"));
                return false;
            }
        }

        // This means the carrier should be docked on a base.
        return true;
    }

    const enum JumpState
    {
        NoJumpPath,
        DestinationReached,
        Jumping,
        Error
    };

    static JumpState TryJumpToNextSystem(const uint clientId)
    {
        if (!undockPathByClientId.contains(clientId))
            return JumpState::NoJumpPath;

        if (undockPathByClientId[clientId].remainingJumpPath.size() == 0)
            return JumpState::DestinationReached;

        uint shipId;
        pub::Player::GetShip(clientId, shipId);
        if (shipId)
        {
            pub::SpaceObj::InstantDock(shipId, undockPathByClientId[clientId].remainingJumpPath.back(), 1);
            undockPathByClientId[clientId].remainingJumpPath.pop_back();
            return JumpState::Jumping;
        }
        return JumpState::Error;
    }

    static void TryReachCarrier(const uint clientId)
    {
        if (!HkIsValidClientID(clientId))
            return;

        const std::string& clientFileName = GetCharacterFileName(clientId);
        if (clientFileName.empty())
            return;
        const uint clientFileNameId = CreateID(clientFileName.c_str());

        if (undockPathByClientId.contains(clientId) && undockPathByClientId[clientId].lastDockLocation.systemId)
        {
            // Try to jump to the carrier.
            switch (TryJumpToNextSystem(clientId))
            {
                // The player reached the destination system and should now be moved to the last docking location.
                case JumpState::DestinationReached:
                {
                    const Location& location = undockPathByClientId[clientId].lastDockLocation;
                    HkRelocateClient(clientId, location.position, location.orientation);
                    undockPathByClientId.erase(clientId);
                    RemoveCharacterFromCarrier(clientFileName);
                    DeleteCharacterCarrierDockLocation(clientFileName);
                    return;
                }

                // The player is still on the way to the target system.
                case JumpState::Jumping:
                    return;
            }

            return;
        }

        if (!IsLinkedToCarrier(clientFileNameId))
            return;

        const uint carrierId = FindCarrierClientIdBySlottedClientId(clientId);
        // Check if the carrier is currently reachable by jumps or base-docking.
        if (!IsCarrierReachable(clientId, carrierId) || !undockPathByClientId.contains(clientId))
        {
            // Beam back to where the player came from.
            const uint shiparchId = carrierAssignmentByCharacterFileNameId[clientFileNameId].carrierShiparchId;
            if (carrierDefinitionByShipArchetypeId.contains(shiparchId))
                HkBeam(ARG_CLIENTID(clientId), carrierDefinitionByShipArchetypeId[shiparchId].baseNickname);
            return;
        }

        // Check if the carrier is currently docked.
        uint carrierShipId;
        pub::Player::GetShip(carrierId, carrierShipId);
        if (!carrierShipId)
        {
            std::wstring baseName;
            // Beam the player to the same base the carrier currently is docked on.
            uint baseId;
            pub::Player::GetBase(carrierId, baseId);
            // baseId can be empty if carrier just died.
            if (baseId)
                baseName = HkGetBaseNickByID(baseId);
            else
                baseName = lastRegularBaseNameByCharacterFileNameId[CreateID(GetCharacterFileName(carrierId).c_str())];

            if (!baseName.empty())
                HkBeam(ARG_CLIENTID(clientId), baseName);
            return;
        }

        // Try to jump to the carrier.
        switch (TryJumpToNextSystem(clientId))
        {
            // The player reached the destination system and should now be moved to the carrier.
            case JumpState::DestinationReached:
            {
                uint carrierShiparchId;
                pub::Player::GetShipID(carrierId, carrierShiparchId);
                uint carrierShipId;
                pub::Player::GetShip(carrierId, carrierShipId);
                if (!carrierShiparchId || !carrierShipId || !carrierDefinitionByShipArchetypeId.contains(carrierShiparchId))
                    return;
                Vector carrierVector;
                Matrix carrierRotation;
                pub::SpaceObj::GetLocation(carrierShipId, carrierVector, carrierRotation);
                const auto& dockOffset = carrierDefinitionByShipArchetypeId[carrierShiparchId].dockOffset;
                TranslateY(carrierVector, carrierRotation, dockOffset[0]);
                TranslateZ(carrierVector, carrierRotation, dockOffset[1]);
                TranslateX(carrierVector, carrierRotation, dockOffset[2]);
                HkRelocateClient(clientId, carrierVector, carrierRotation);
                undockPathByClientId.erase(clientId);
                if (carrierAssignmentByCharacterFileNameId[clientFileNameId].assigned)
                    SaveCharacterDockState(clientFileName, DockState::Undocked);
                else
                {
                    RemoveCharacterFromCarrier(clientFileName);
                    DeleteCharacterCarrierDockLocation(clientFileName);
                }
                return;
            }

            // The player is still on the way to the target system.
            case JumpState::Jumping:
                return;
        }

        return;
    }

    void __stdcall LaunchComplete_After(unsigned int objectId, unsigned int shipId)
    {
        const uint clientId = HkGetClientIDByShip(shipId);
        if (!clientId)
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }

        const std::string& fileName = GetCharacterFileName(clientId);
        if (fileName.empty())
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }

        const uint fileNameId = CreateID(fileName.c_str());

        // Check if the undock happened from a carrier base.
        uint baseId;
        pub::SpaceObj::GetDockingTarget(objectId, baseId);
        if (!IsCarrierBase(baseId))
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }

        uint targetSystemId = 0;
        undockPathByClientId[clientId].lastDockLocation.systemId = 0;
        // Check if the player should try to start from a carrier.
        if (IsLinkedToCarrier(fileNameId))
        {
            const uint carrierId = FindCarrierClientIdBySlottedClientId(clientId);
            if (carrierId)
            {
                uint carrierSystemId;
                pub::Player::GetSystem(carrierId, carrierSystemId);
                if (carrierSystemId)
                    targetSystemId = carrierSystemId;
            }
        }

        // If the carrier is not found in any system try to return to the last dock-in position.
        if (!targetSystemId && dockLocationByCharacterFileNameIds.contains(fileNameId))
        {
            // Target the location where the player docked to the carrier.
            undockPathByClientId[clientId].lastDockLocation = dockLocationByCharacterFileNameIds[fileNameId];
            targetSystemId = undockPathByClientId[clientId].lastDockLocation.systemId;
        }

        // If still no target location was found then directly beam back to the last regular base.
        if (!targetSystemId && lastRegularBaseNameByCharacterFileNameId.contains(fileNameId))
        {
            if (HkBeam(ARG_CLIENTID(clientId), lastRegularBaseNameByCharacterFileNameId[fileNameId]) == HKE_OK)
            {
                undockPathByClientId.erase(clientId);
                returncode = DEFAULT_RETURNCODE;
                return;
            }
        }

        // Check if a path to the target system exists.
        if (shortestJumpObjectPathToTargets.contains(targetSystemId))
        {
            undockPathByClientId[clientId].remainingJumpPath = shortestJumpObjectPathToTargets[targetSystemId];
        }
        else
        {
            PrintUserCmdText(clientId, L"BUG: Cannot find target system. Report to developer: " + std::to_wstring(targetSystemId));
            undockPathByClientId.erase(clientId);
        }

        TryReachCarrier(clientId);
        returncode = DEFAULT_RETURNCODE;
    }

    int __cdecl Dock_Call(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, enum DOCK_HOST_RESPONSE response)
    {
        const uint clientId = HkGetClientIDByShip(shipId);
        if (!clientId)
        {
            returncode = DEFAULT_RETURNCODE;
            return 0;
        }

        carrierClientIdInJump.erase(clientId);

        // Cancel docking?
        if (dockPortIndex == -1)
        {
            returncode = DEFAULT_RETURNCODE;
            return 0;
        }

        uint shiparchId;
        pub::Player::GetShipID(clientId, shiparchId);
        if (!shiparchId)
        {
            returncode = DEFAULT_RETURNCODE;
            return 0;
        }

        if (carrierDefinitionByShipArchetypeId.contains(shiparchId))
        {
            uint dockTargetType;
            pub::SpaceObj::GetType(dockTargetId, dockTargetType);
            if (dockTargetType == ObjectType::JumpHole || dockTargetType == ObjectType::JumpGate)
                carrierClientIdInJump.insert(clientId);

            returncode = DEFAULT_RETURNCODE;
            return 0;
        }

        returncode = DEFAULT_RETURNCODE;
        return 0;
    }

    void __stdcall JumpInComplete_After(unsigned int systemId, unsigned int shipId)
    {
        uint clientId = HkGetClientIDByShip(shipId);
        if (clientId)
            carrierClientIdInJump.erase(clientId);

        if (undockPathByClientId.contains(clientId))
            TryReachCarrier(clientId);

        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
    {
        carrierClientIdInJump.erase(clientId);
        undockPathByClientId.erase(clientId);

        // Check if the player is related to any carrier.
        const std::string& characterFileName = GetCharacterFileName(clientId);
        const uint characterFileNameId = CreateID(characterFileName.c_str());
        if (IsLinkedToCarrier(characterFileNameId))
        {
            const auto& assignment = carrierAssignmentByCharacterFileNameId[characterFileNameId];
            // Do nothing if fully undocked.
            if (assignment.dockState == DockState::Undocked)
            {
                returncode = DEFAULT_RETURNCODE;
                return;
            }

            // TODO This will cause the player never be able to leave the base. TryReachCarrier will always teleport back when no undockPath given
            // Shortcut check if carrier is even reachable. Leads to instant beam back.
            //uint carrierId = FindCarrierClientIdBySlottedClientId(clientId);
            //if (carrierId && !IsCarrierReachable(clientId, carrierId))
            //    SaveCharacterDockState(fileName, DockState::Undocking);

            // When logging into space.
            if (assignment.dockState == DockState::Undocking)
                TryReachCarrier(clientId);
            // When coming just out of a base. LaunchComplete does the rest.
            else if (assignment.dockState == DockState::Docked)
                SaveCharacterDockState(characterFileName, DockState::Undocking);
        }
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall BaseEnter_After(unsigned int baseId, unsigned int clientId)
    {
        undockPathByClientId.erase(clientId);

        const std::string& characterFileName = GetCharacterFileName(clientId);
        if (characterFileName.empty())
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }
        const uint characterFileNameId = CreateID(characterFileName.c_str());

        if (IsCarrierBase(baseId))
        {
            SaveCharacterDockState(characterFileName, DockState::Docked);
        }
        else
        {
            SaveCharacterLastRegularBase(characterFileName, baseId);
            // Unassign the player from a carrier once docked to another base than the carrier's interior.
            if (carrierAssignmentByCharacterFileNameId.contains(characterFileNameId))
                RemoveCharacterFromCarrier(characterFileName);
            if (dockLocationByCharacterFileNameIds.contains(characterFileNameId))
                DeleteCharacterCarrierDockLocation(characterFileName);

            if (IsCarrier(clientId))
            {
                for (const auto& assignment : carrierAssignmentByCharacterFileNameId)
                {
                    if (assignment.second.carrierFileName == characterFileName)
                    {
                        SaveCharacterLastRegularBase(assignment.second.characterFileName, baseId);
                        DeleteCharacterCarrierDockLocation(assignment.second.characterFileName);
                    }
                }
            }
        }
        returncode = DEFAULT_RETURNCODE;
    }

    static void ClearCarrier(const std::string carrierFileName)
    {
        const uint carrierFileNameId = CreateID(carrierFileName.c_str());
        std::vector<std::string> characterFileNamesToRemove;
        for (const auto& assignment : carrierAssignmentByCharacterFileNameId)
        {
            if (!assignment.second.carrierFileName.empty() && CreateID(assignment.second.carrierFileName.c_str()) == carrierFileNameId)
                characterFileNamesToRemove.push_back(assignment.second.characterFileName);
        }
        for (const std::string& characterFileName : characterFileNamesToRemove)
            RemoveCharacterFromCarrier(characterFileName);
    }

    static void ClearCarrier(const uint clientId)
    {
        const std::string& fileName = GetCharacterFileName(clientId);
        if (!fileName.empty())
            ClearCarrier(fileName);
    }

    void __stdcall ReqShipArch_After(unsigned int archetypeId, unsigned int clientId)
    {
        ClearCarrier(clientId);
        returncode = DEFAULT_RETURNCODE;
    }

    static void DeleteCharacterFromRecords(const std::string& characterFileName)
    {
        const uint characterFileNameId = CreateID(characterFileName.c_str());
        carrierAssignmentByCharacterFileNameId.erase(characterFileNameId);
        dockLocationByCharacterFileNameIds.erase(characterFileNameId);
        lastRegularBaseNameByCharacterFileNameId.erase(characterFileNameId);
        IniDelSection(GetCarrierMapFilePath(), characterFileName);

        // Throw off every character that is docked here.
        ClearCarrier(characterFileName);
    }

    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId)
    {
        std::wstring characterFileNameWS;
        if (HkGetCharFileName(info.wszCharname, characterFileNameWS) != HKE_OK)
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }
        DeleteCharacterFromRecords(wstos(characterFileNameWS));
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId)
    {
        const std::string characterFileName = std::string(characterId.charFilename).substr(0, 11);
        DeleteCharacterFromRecords(characterFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    static void MoveCharacterInRecords(const std::string& oldCharacterFileName, const std::string& newCharacterFileName)
    {
        const uint oldCharacterFileNameId = CreateID(oldCharacterFileName.c_str());
        const uint newCharacterFileNameId = CreateID(newCharacterFileName.c_str());

        if (carrierAssignmentByCharacterFileNameId.contains(oldCharacterFileNameId))
        {
            carrierAssignmentByCharacterFileNameId[newCharacterFileNameId] = carrierAssignmentByCharacterFileNameId[oldCharacterFileNameId];
            carrierAssignmentByCharacterFileNameId[newCharacterFileNameId].characterFileName = newCharacterFileName;
            carrierAssignmentByCharacterFileNameId.erase(oldCharacterFileNameId);
        }

        if (dockLocationByCharacterFileNameIds.contains(oldCharacterFileNameId))
        {
            dockLocationByCharacterFileNameIds[newCharacterFileNameId] = dockLocationByCharacterFileNameIds[oldCharacterFileNameId];
            dockLocationByCharacterFileNameIds.erase(oldCharacterFileNameId);
        }

        if (lastRegularBaseNameByCharacterFileNameId.contains(oldCharacterFileNameId))
        {
            lastRegularBaseNameByCharacterFileNameId[newCharacterFileNameId] = lastRegularBaseNameByCharacterFileNameId[oldCharacterFileNameId];
            lastRegularBaseNameByCharacterFileNameId.erase(oldCharacterFileNameId);
        }

        const std::string carrierFilePath = GetCarrierMapFilePath();
        std::list<INISECTIONVALUE> keyValues;
        IniGetSection(carrierFilePath, oldCharacterFileName, keyValues);
        for (const INISECTIONVALUE keyValue : keyValues)
            IniWrite(carrierFilePath, newCharacterFileName, keyValue.scKey, keyValue.scValue);
        IniDelSection(carrierFilePath, oldCharacterFileName);

        // Re-assign all clients that are docked to this character.
        for (const auto& assignment : carrierAssignmentByCharacterFileNameId)
        {
            if (!assignment.second.carrierFileName.empty() && CreateID(assignment.second.carrierFileName.c_str()) == oldCharacterFileNameId)
            {
                carrierAssignmentByCharacterFileNameId[assignment.first].carrierFileName = newCharacterFileName;
                IniWrite(carrierFilePath, assignment.second.characterFileName, "assignment", newCharacterFileName + ", " + (assignment.second.assigned ? "true" : "false"));
            }
        }
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
            std::wstring newCharacterFileName;
            if (HkGetCharFileName(newCharname, newCharacterFileName) == HKE_OK)
                MoveCharacterInRecords(wstos(oldCharacterFileName), wstos(newCharacterFileName));
        }
        oldCharacterFileName = L"";
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }

    void UserCmd_Dock(const uint clientId, const std::wstring& wscParam)
    {
        TryDock(clientId);
    }
}