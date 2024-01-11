#include "Main.h"

namespace Mark
{
    // Contains a set of currently market targets (the current system's) per client id.
    static std::map<uint, std::set<uint>> currentlyMarkedObjectsPerClient;

    // Contains a set of marked targets per system per character file name.
    static std::map<std::wstring, std::map<uint, std::set<uint>>> markedObjectsPerCharacter;

    // Contains a set of object ids which can be targetted for marking, but will not be marked visibly to the player.
    static std::set<uint> hiddenMarkedIds;

    /**
    * Generally we assume that objects which will be marked MUST exist for the client and server. Otherwise it may cause a crash.
    * Objects which have already been marked can be safely unmarked.
    */

    bool IsTargetInSameSystemAsPlayer(const uint clientId, const uint targetId)
    {
        uint clientSystemId, targetSystemId;
        pub::Player::GetSystem(clientId, clientSystemId);
        pub::SpaceObj::GetSystem(targetId, targetSystemId);
        return clientSystemId != 0 && clientSystemId == targetSystemId;
    }

    void TryRemoveInvisibleMarkId(const uint id)
    {
        if (pub::SpaceObj::ExistsAndAlive(id) != 0) // 0 -> true
            hiddenMarkedIds.erase(id);
    }

    void HideObjectMark(const uint id)
    {
        if (hiddenMarkedIds.contains(id))
            return;

        hiddenMarkedIds.insert(id);
        for (const auto& clientIdTargetIds : currentlyMarkedObjectsPerClient)
        {
            for (const uint targetId : clientIdTargetIds.second)
            {
                if (targetId == id)
                    pub::Player::MarkObj(clientIdTargetIds.first, targetId, 0);
            }
        }
    }

    void ShowObjectMark(const uint id)
    {
        if (!hiddenMarkedIds.contains(id))
            return;

        hiddenMarkedIds.erase(id);
        for (const auto& clientIdTargetIds : currentlyMarkedObjectsPerClient)
        {
            for (const uint targetId : clientIdTargetIds.second)
            {
                if (targetId == id && IsTargetInSameSystemAsPlayer(clientIdTargetIds.first, targetId))
                    pub::Player::MarkObj(clientIdTargetIds.first, targetId, 1);
            }
        }
    }

    bool TryMarkObject(const uint clientId, const uint targetId)
    {
        if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return false;

        // Prevent from having the own ship marked.
        uint shipId;
        pub::Player::GetShip(clientId, shipId);
        if (shipId == targetId)
            return false;

        if (IsTargetInSameSystemAsPlayer(clientId, targetId) && (!currentlyMarkedObjectsPerClient.contains(clientId) || !currentlyMarkedObjectsPerClient[clientId].contains(targetId)))
        {
            currentlyMarkedObjectsPerClient[clientId].insert(targetId);
            if (!hiddenMarkedIds.contains(targetId))
            {
                pub::Player::MarkObj(clientId, targetId, 1);
                pub::Audio::PlaySoundEffect(clientId, CreateID("ui_select_add"));
            }
            return true;
        }
        return false;
    }

    bool TryUnmarkObject(const uint clientId, const uint targetId)
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

    void UnmarkAllObjects(const uint clientId, const boolean noSound = false)
    {
        if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
            return;

        if (currentlyMarkedObjectsPerClient.contains(clientId))
        {
            if (!noSound && currentlyMarkedObjectsPerClient[clientId].size() > 0)
                pub::Audio::PlaySoundEffect(clientId, CreateID("ui_select_remove"));

            for (const uint& targetId : currentlyMarkedObjectsPerClient[clientId])
                pub::Player::MarkObj(clientId, targetId, 0);

            currentlyMarkedObjectsPerClient[clientId].clear();
        }
    }

    void RegisterObject(const uint clientId, const uint targetId)
    {
        if (!HkIsValidClientID(clientId))
            return;

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;
        uint targetSystemId;
        pub::SpaceObj::GetSystem(targetId, targetSystemId);
        if (targetSystemId)
            markedObjectsPerCharacter[characterFileNameWS][targetSystemId].insert(targetId);
    }

    void UnregisterObject(uint clientId, uint targetId)
    {
        TryRemoveInvisibleMarkId(targetId);

        if (!HkIsValidClientID(clientId))
            return;

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;
        uint targetSystemId;
        pub::SpaceObj::GetSystem(targetId, targetSystemId);
        if (targetSystemId && markedObjectsPerCharacter.contains(characterFileNameWS) && markedObjectsPerCharacter[characterFileNameWS].contains(targetSystemId))
            markedObjectsPerCharacter[characterFileNameWS][targetSystemId].erase(targetId);
    }

    void UpdateAndMarkCurrentSystemMarks(const uint clientId)
    {
        for (const uint id : hiddenMarkedIds)
            TryRemoveInvisibleMarkId(id);

        if (!HkIsValidClientID(clientId))
            return;

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;

        const bool characterInSpace = !HkIsInCharSelectMenu(clientId);

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
                    if (characterInSpace)
                    {
                        currentlyMarkedObjectsPerClient[clientId].insert(targetId);
                        if (!hiddenMarkedIds.contains(targetId))
                            pub::Player::MarkObj(clientId, targetId, 1);
                    }
                    ++iterator;
                }
            }
        }
    }

    void MarkAndRegisterObject(const uint clientId, const uint targetId)
    {
        TryMarkObject(clientId, targetId);
        RegisterObject(clientId, targetId);
    }

    void UnmarkAndUnregisterObject(const uint clientId, const uint targetId)
    {
        TryUnmarkObject(clientId, targetId);
        UnregisterObject(clientId, targetId);
    }

    void UnmarkAndUnregisterObjectForEveryone(const uint targetId)
    {
        TryRemoveInvisibleMarkId(targetId);

        PlayerData* playerData = 0;
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

    std::list<GROUP_MEMBER> GetGroupMemberClientIds(const uint clientId)
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
        if (!shipId)
            return;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return;
        MarkAndRegisterObject(clientId, targetId);
    }

    void UserCmd_GroupMark(uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return;
        const auto groupMembers = GetGroupMemberClientIds(clientId);
        for (const auto& member : groupMembers)
            MarkAndRegisterObject(member.iClientID, targetId);
    }

    void UserCmd_UnMark(uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return;
        UnmarkAndUnregisterObject(clientId, targetId);
    }

    void UserCmd_UnGroupMark(uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return;
        const auto groupMembers = GetGroupMemberClientIds(clientId);
        for (const auto& member : groupMembers)
            UnmarkAndUnregisterObject(member.iClientID, targetId);
    }

    void UserCmd_UnMarkAll(uint clientId, const std::wstring& wscParam)
    {
        UnmarkAllObjects(clientId);

        std::wstring characterFileNameWS;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileNameWS) != HKE_OK)
            return;

        markedObjectsPerCharacter[characterFileNameWS].clear();
    }

    void __stdcall SystemSwitchOutComplete_After(unsigned int shipId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;
        
        UnmarkAllObjects(clientId, true);
        UpdateAndMarkCurrentSystemMarks(clientId);
    }

    void __stdcall PlayerLaunch_After(unsigned int ship, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (currentlyMarkedObjectsPerClient.contains(clientId))
            currentlyMarkedObjectsPerClient[clientId].clear();
        UpdateAndMarkCurrentSystemMarks(clientId);
    }

    void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (currentlyMarkedObjectsPerClient.contains(clientId))
            currentlyMarkedObjectsPerClient[clientId].clear();
    }
}