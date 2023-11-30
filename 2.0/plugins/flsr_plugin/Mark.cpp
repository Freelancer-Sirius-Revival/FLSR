#include "Main.h"

namespace Mark
{
    // Contains a set of currently market targets (the current system's) per client id.
    static std::map<uint, std::set<uint>> currentlyMarkedObjectsPerClient;

    // Contains a set of marked targets per system per character file name.
    static std::map<std::wstring, std::map<uint, std::set<uint>>> markedObjectsPerCharacter;

    bool TryMarkObject(uint clientId, uint targetId)
    {
        if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return false;

        // Prevent from having the own ship marked.
        uint shipId;
        pub::Player::GetShip(clientId, shipId);
        if (shipId == targetId)
            return false;

        uint clientSystemId, targetSystemId;
        pub::Player::GetSystem(clientId, clientSystemId);
        pub::SpaceObj::GetSystem(targetId, targetSystemId);
        if (clientSystemId == targetSystemId && (!currentlyMarkedObjectsPerClient.contains(clientId) || !currentlyMarkedObjectsPerClient[clientId].contains(targetId)))
        {
            if (Cloak::FindShipCloakState(targetId) == Cloak::CloakState::Cloaked)
                return false;

            pub::Player::MarkObj(clientId, targetId, 1);
            currentlyMarkedObjectsPerClient[clientId].insert(targetId);
            pub::Audio::PlaySoundEffect(clientId, CreateID("ui_select_add"));
            return true;
        }
        return false;
    }

    bool TryUnmarkObject(uint clientId, uint targetId)
    {
        if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return false;

        if (currentlyMarkedObjectsPerClient.contains(clientId) && currentlyMarkedObjectsPerClient[clientId].contains(targetId))
        {
            pub::Player::MarkObj(clientId, targetId, 0);
            currentlyMarkedObjectsPerClient[clientId].erase(targetId);
            pub::Audio::PlaySoundEffect(clientId, CreateID("ui_select_remove"));
            return true;
        }
        return false;
    }

    void UnmarkAllObjects(uint clientId)
    {
        if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return;

        if (currentlyMarkedObjectsPerClient.contains(clientId))
        {
            for (const uint& targetId : currentlyMarkedObjectsPerClient[clientId])
            {
                pub::Player::MarkObj(clientId, targetId, 0);
            }
            currentlyMarkedObjectsPerClient[clientId].clear();
            pub::Audio::PlaySoundEffect(clientId, CreateID("ui_select_remove"));
        }
    }

    void RegisterObject(uint clientId, uint targetId)
    {
        if (!HkIsValidClientID(clientId))
            return;

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;
        uint targetSystemId;
        pub::SpaceObj::GetSystem(targetId, targetSystemId);
        markedObjectsPerCharacter[characterFileNameWS][targetSystemId].insert(targetId);
    }

    void UnregisterObject(uint clientId, uint targetId)
    {
        if (!HkIsValidClientID(clientId))
            return;

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;
        uint targetSystemId;
        pub::SpaceObj::GetSystem(targetId, targetSystemId);
        if (markedObjectsPerCharacter.contains(characterFileNameWS) && markedObjectsPerCharacter[characterFileNameWS].contains(targetSystemId))
            markedObjectsPerCharacter[characterFileNameWS][targetSystemId].erase(targetId);
    }

    void UpdateAndMarkCurrentSystemMarks(uint clientId)
    {
        if (!HkIsValidClientID(clientId))
            return;

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;

        const bool characterNotInSpace = HkIsInCharSelectMenu(clientId);

        uint clientSystemId;
        pub::Player::GetSystem(clientId, clientSystemId);
        if (clientSystemId && markedObjectsPerCharacter.contains(characterFileNameWS) && markedObjectsPerCharacter[characterFileNameWS].contains(clientSystemId))
        {
            auto& list = markedObjectsPerCharacter[characterFileNameWS][clientSystemId];
            for (auto iterator = list.begin(); iterator != list.end();)
            {
                const uint targetId = *iterator;
                if (pub::SpaceObj::ExistsAndAlive(targetId) != 0) // 0 -> true
                {
                    list.erase(iterator);
                }
                else
                {
                    if (!characterNotInSpace)
                    {
                        pub::Player::MarkObj(clientId, targetId, 1);
                        currentlyMarkedObjectsPerClient[clientId].insert(targetId);
                    }
                    ++iterator;
                }
            }
        }
    }

    void MarkAndRegisterObject(uint clientId, uint targetId)
    {
        TryMarkObject(clientId, targetId);
        RegisterObject(clientId, targetId);
    }

    void UnmarkAndUnregisterObject(uint clientId, uint targetId)
    {
        TryUnmarkObject(clientId, targetId);
        UnregisterObject(clientId, targetId);
    }

    void UnmarkAndUnregisterObjectForEveryone(uint targetId)
    {
        struct PlayerData* playerData = 0;
        while (playerData = Players.traverse_active(playerData))
        {
            const uint clientId = HkGetClientIdFromPD(playerData);
            TryUnmarkObject(clientId, targetId);
        }

        for (auto& characterSystemObjects : markedObjectsPerCharacter)
        {
            auto& objectsPerSystem = characterSystemObjects.second;
            for (auto& systemObjects : objectsPerSystem)
                systemObjects.second.erase(targetId);
        }
    }

    std::list<GROUP_MEMBER> GetGroupMemberClientIds(uint clientId)
    {
        std::list<GROUP_MEMBER> members;
        if (!HkIsValidClientID(clientId))
            return members;

        HkGetGroupMembers(ARG_CLIENTID(clientId), members);
        return members;
    }

    void UserCmd_Mark(uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        pub::SpaceObj::GetTarget(shipId, targetId);
        MarkAndRegisterObject(clientId, targetId);
    }

    void UserCmd_GroupMark(uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        pub::SpaceObj::GetTarget(shipId, targetId);
        const auto groupMembers = GetGroupMemberClientIds(clientId);
        for (const auto& member : groupMembers)
        {
            MarkAndRegisterObject(member.iClientID, targetId);
        }
    }

    void UserCmd_UnMark(uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        pub::SpaceObj::GetTarget(shipId, targetId);
        UnmarkAndUnregisterObject(clientId, targetId);
    }

    void UserCmd_UnGroupMark(uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        pub::SpaceObj::GetTarget(shipId, targetId);
        const auto groupMembers = GetGroupMemberClientIds(clientId);
        for (const auto& member : groupMembers)
        {
            UnmarkAndUnregisterObject(member.iClientID, targetId);
        }
    }

    void UserCmd_UnMarkAll(uint clientId, const std::wstring& wscParam)
    {
        UnmarkAllObjects(clientId);

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;

        markedObjectsPerCharacter[characterFileNameWS].clear();
    }

    void __stdcall JumpInComplete(unsigned int systemId, unsigned int shipId)
    {
        returncode = DEFAULT_RETURNCODE;

        uint clientId = HkGetClientIDByShip(shipId);
        if (!clientId)
            return;

        UnmarkAllObjects(clientId);
        UpdateAndMarkCurrentSystemMarks(clientId);
    }

    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        currentlyMarkedObjectsPerClient[clientId].clear();
        UpdateAndMarkCurrentSystemMarks(clientId);
    }

    void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        currentlyMarkedObjectsPerClient[clientId].clear();
    }
}