#include "Mark.h"
#include <queue>
#include "Plugin.h"
#include <ranges>

namespace Mark
{
    const uint UI_SELECT_ID = CreateID("ui_select_add");
    const uint UI_UNSELECT_ID = CreateID("ui_select_remove");

    struct Marker
    {
        std::unordered_map<uint, std::unordered_set<uint>> objectIdsBySystemId;
        std::unordered_set<std::wstring> charFileNames;
    };
    std::unordered_map<std::wstring, Marker> markersByCharFileName;
    std::unordered_set<uint> invisibleObjectIds;

    static uint FindClientIdByCharFileName(const std::wstring& characterFileName)
    {
        PlayerData* playerData = nullptr;
        while (playerData = Players.traverse_active(playerData))
        {
            std::wstring otherCharacterFileName;
            if (HkGetCharFileName(ARG_CLIENTID(playerData->iOnlineID), otherCharacterFileName) == HKE_OK && otherCharacterFileName.compare(characterFileName) == 0)
                return playerData->iOnlineID;
        }
        return -1;
    }

    static std::wstring GetCharFileName(const uint clientId)
    {
        std::wstring characterFileName;
        if (HkGetCharFileName(ARG_CLIENTID(clientId), characterFileName) != HKE_OK)
            return L"";
        return characterFileName;
    }

    void Mark(const uint clientId, const uint targetId)
    {
        IObjRW* targetInspect = nullptr;
        StarSystem* system;
        if (!GetShipInspect(targetId, targetInspect, system) || !targetInspect)
            return;

        // Only allow ships, solars,and loot. The rest is not supported by the client itself.
        if (!(targetInspect->cobj->objectClass & CObject::CEQOBJ_MASK) || !(targetInspect->cobj->objectClass & CObject::CLOOT_OBJECT))
            return;

        auto& marker = markersByCharFileName[GetCharFileName(clientId)];
        const bool newlyInserted = targetInspect->is_player()
                                        ? marker.charFileNames.insert(GetCharFileName(targetInspect->cobj->ownerPlayer)).second
                                        : marker.objectIdsBySystemId[targetInspect->cobj->system].insert(targetId).second;

        if (invisibleObjectIds.contains(targetId))
            return;
        uint shipId, systemId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;
        pub::Player::GetSystem(clientId, systemId);
        if (targetInspect->cobj->system == systemId)
        {
            pub::Player::MarkObj(clientId, targetId, 1);
            if (newlyInserted)
                pub::Audio::PlaySoundEffect(clientId, UI_SELECT_ID);
        }
    }

    static void Unmark(const std::wstring charFileName, const uint targetId)
    {
        const auto& markerEntry = markersByCharFileName.find(charFileName);
        if (markerEntry == markersByCharFileName.end())
            return;

        IObjRW* targetInspect = nullptr;
        StarSystem* system;
        if (!GetShipInspect(targetId, targetInspect, system) || !targetInspect)
            return;

        bool removed = false;
        if (targetInspect->is_player())
            removed = markerEntry->second.charFileNames.erase(GetCharFileName(targetInspect->cobj->ownerPlayer)) > 0;
        else if (const auto& objectIdsBySystem = markerEntry->second.objectIdsBySystemId.find(targetInspect->cobj->system); objectIdsBySystem != markerEntry->second.objectIdsBySystemId.end())
            removed = objectIdsBySystem->second.erase(targetId) > 0;
        if (!removed)
            return;

        if (markerEntry->second.charFileNames.empty() && markerEntry->second.objectIdsBySystemId.empty())
            markersByCharFileName.erase(markerEntry);

        const uint clientId = FindClientIdByCharFileName(charFileName);
        if (!HkIsValidClientID(clientId))
            return;
        uint shipId, systemId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;
        pub::Player::GetSystem(clientId, systemId);
        if (targetInspect->cobj->system == systemId)
        {
            pub::Player::MarkObj(clientId, targetId, 0);
            pub::Audio::PlaySoundEffect(clientId, UI_UNSELECT_ID);
        }
    }

    void Unmark(const uint clientId, const uint targetId)
    {
        Unmark(GetCharFileName(clientId), targetId);
    }

    static void UnmarkAll(const uint clientId)
    {
        const auto& markerEntry = markersByCharFileName.find(GetCharFileName(clientId));
        if (markerEntry == markersByCharFileName.end())
            return;

        uint shipId, systemId;
        pub::Player::GetShip(clientId, shipId);
        pub::Player::GetSystem(clientId, systemId);
        if (shipId && systemId)
        {
            bool unmarked = false;
            for (const auto& charFileName : markerEntry->second.charFileNames)
            {
                const uint targetClientId = FindClientIdByCharFileName(charFileName);
                if (!HkIsValidClientID(targetClientId))
                    continue;
                uint targetShipId, targetSystemId;
                pub::Player::GetShip(targetClientId, targetShipId);
                if (!targetShipId)
                    continue;
                pub::Player::GetSystem(targetClientId, targetSystemId);
                if (targetSystemId == systemId)
                {
                    unmarked = true;
                    pub::Player::MarkObj(clientId, targetShipId, 0);
                }
            }
            if (const auto& objectIdsBySystem = markerEntry->second.objectIdsBySystemId.find(systemId); objectIdsBySystem != markerEntry->second.objectIdsBySystemId.end())
            {
                for (const auto& objectId : objectIdsBySystem->second)
                {
                    unmarked = true;
                    pub::Player::MarkObj(clientId, objectId, 0);
                }
            }
            if (unmarked)
                pub::Audio::PlaySoundEffect(clientId, UI_UNSELECT_ID);
        }

        markersByCharFileName.erase(markerEntry);
    }

    static void UnmarkEverywhere(const uint objectId)
    {
        const auto ks = std::views::keys(markersByCharFileName);
        const std::vector<std::wstring> keys{ ks.begin(), ks.end() };
        for (const auto& characterName : keys)
            Unmark(characterName, objectId);
    }

    static bool IsMarked(const uint clientId, const uint targetId)
    {
        const auto& markerEntry = markersByCharFileName.find(GetCharFileName(clientId));
        if (markerEntry == markersByCharFileName.end())
            return false;

        IObjRW* targetInspect = nullptr;
        StarSystem* system;
        if (!GetShipInspect(targetId, targetInspect, system) || !targetInspect)
            return false;

        bool marked = false;
        if (targetInspect->is_player())
            marked = markerEntry->second.charFileNames.contains(GetCharFileName(targetInspect->cobj->ownerPlayer));
        else if (const auto& objectIdsBySystem = markerEntry->second.objectIdsBySystemId.find(targetInspect->cobj->system); objectIdsBySystem != markerEntry->second.objectIdsBySystemId.end())
            marked = objectIdsBySystem->second.contains(targetId);
        return marked;
    }

    static void MarkForCurrentSystem(const uint clientId)
    {
        uint systemId;
        pub::Player::GetSystem(clientId, systemId);
        if (const auto& entry = markersByCharFileName.find(GetCharFileName(clientId)); entry != markersByCharFileName.end())
        {
            // All ships will be dealt with by Send_FLPACKET_SERVER_CREATESHIP_AFTER
            if (const auto& objectIdsBySystem = entry->second.objectIdsBySystemId.find(systemId); objectIdsBySystem != entry->second.objectIdsBySystemId.end())
            {
                for (const auto& objectId : objectIdsBySystem->second)
                    if (!invisibleObjectIds.contains(objectId))
                        pub::Player::MarkObj(clientId, objectId, 1);
            }
        }
    }

    static void RefreshInvisibleObject(const uint objectId)
    {
        IObjRW* targetInspect = nullptr;
        StarSystem* system;
        if (!GetShipInspect(objectId, targetInspect, system) || !targetInspect)
            return;
        for (const auto& charFileEntry : markersByCharFileName)
        {
            const uint clientId = FindClientIdByCharFileName(charFileEntry.first);
            if (HkIsValidClientID(clientId) && IsMarked(clientId, objectId))
            {
                uint shipId, systemId;
                pub::Player::GetShip(clientId, shipId);
                if (!shipId)
                    continue;
                pub::Player::GetSystem(clientId, systemId);
                if (targetInspect->cobj->system == systemId)
                    pub::Player::MarkObj(clientId, objectId, invisibleObjectIds.contains(objectId) ? 0 : 1);
            }
        }
    }

    void AddInvisibleObject(const uint objectId)
    {
        invisibleObjectIds.insert(objectId);
        RefreshInvisibleObject(objectId);
    }

    void RemoveInvisibleObject(const uint objectId)
    {
        invisibleObjectIds.erase(objectId);
        RefreshInvisibleObject(objectId);
    }

    static std::list<GROUP_MEMBER> GetGroupMemberClientIds(const uint clientId)
    {
        std::list<GROUP_MEMBER> members;
        if (!HkIsValidClientID(clientId))
            return members;

        HkGetGroupMembers(ARG_CLIENTID(clientId), members);
        return members;
    }

    void UserCmd_Mark(const uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return;

        IsMarked(clientId, targetId) ? Unmark(clientId, targetId) : Mark(clientId, targetId);
    }

    void UserCmd_UnMark(const uint clientId, const std::wstring& wscParam)
    {
        uint shipId, targetId;
        pub::Player::GetShip(clientId, shipId);
        if (!shipId)
            return;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return;

        Unmark(clientId, targetId);
    }

    void UserCmd_GroupMark(const uint clientId, const std::wstring& wscParam)
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
            Mark(member.iClientID, targetId);
    }

    void UserCmd_UnGroupMark(const uint clientId, const std::wstring& wscParam)
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
            Unmark(member.iClientID, targetId);
    }

    void UserCmd_UnMarkAll(const uint clientId, const std::wstring& wscParam)
    {
        UnmarkAll(clientId);
    }

    void __stdcall SystemSwitchOutComplete_After(unsigned int shipId, unsigned int clientId)
    {
        MarkForCurrentSystem(clientId);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall PlayerLaunch_After(unsigned int shipId, unsigned int clientId)
    {
        MarkForCurrentSystem(clientId);
        returncode = DEFAULT_RETURNCODE;
    }

    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship)
    {
        if (const auto& entry = markersByCharFileName.find(GetCharFileName(clientId)); entry != markersByCharFileName.end())
        {
            if (IsMarked(clientId, ship.iSpaceID))
                Mark(clientId, ship.iSpaceID);
        }
        returncode = DEFAULT_RETURNCODE;
        return true;
    }

    bool Send_FLPACKET_SERVER_CREATESOLAR_AFTER(uint clientId, FLPACKET_CREATESOLAR& solar)
    {
        if (const auto& entry = markersByCharFileName.find(GetCharFileName(clientId)); entry != markersByCharFileName.end())
        {
            if (IsMarked(clientId, solar.iSpaceID))
                Mark(clientId, solar.iSpaceID);
        }
        returncode = DEFAULT_RETURNCODE;
        return true;
    }

    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
    {
        invisibleObjectIds.erase(killedObject->get_id());
        UnmarkEverywhere(killedObject->cobj->get_id());
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
    {
        invisibleObjectIds.erase(killedObject->get_id());
        if (killed || !killedObject->is_player())
            UnmarkEverywhere(killedObject->cobj->get_id());
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall LootDestroyed(const IObjRW* killedObject)
    {
        UnmarkEverywhere(killedObject->cobj->get_id());
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId)
    {
        if (std::wstring charFileName; HkGetCharFileName(info.wszCharname, charFileName) == HKE_OK)
        {
            markersByCharFileName.erase(charFileName);
            for (auto& entry : markersByCharFileName)
                entry.second.charFileNames.erase(charFileName);
        }
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId)
    {
        const std::wstring charFileName = stows(std::string(characterId.charFilename).substr(0, 11));
        markersByCharFileName.erase(charFileName);
        for (auto& entry : markersByCharFileName)
            entry.second.charFileNames.erase(charFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    std::wstring oldCharFileName = L"";

    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        oldCharFileName = L"";
        HkGetCharFileName(charname, oldCharFileName);
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }

    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        if (!oldCharFileName.empty())
        {
            std::wstring newCharFileName;
            if (HkGetCharFileName(newCharname, newCharFileName) == HKE_OK)
            {
                if (const auto& markerByCharFileNameEntry = markersByCharFileName.find(oldCharFileName); markerByCharFileNameEntry != markersByCharFileName.end())
                {
                    markersByCharFileName[newCharFileName] = markerByCharFileNameEntry->second;
                    markersByCharFileName.erase(markerByCharFileNameEntry);
                }
                for (auto& markerByCharFileNameEntry : markersByCharFileName)
                {
                    if (const auto& charFileNameEntry = markerByCharFileNameEntry.second.charFileNames.find(oldCharFileName); charFileNameEntry != markerByCharFileNameEntry.second.charFileNames.end())
                    {
                        markerByCharFileNameEntry.second.charFileNames.insert(newCharFileName);
                        markerByCharFileNameEntry.second.charFileNames.erase(charFileNameEntry);
                    }
                }
            }
        }
        oldCharFileName = L"";
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }
}