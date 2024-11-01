#include "main.h"

/**
* IFF PLUGIN - setting players hostile, neutral, or allied in space.
* by Skotty
* 
* Cases:
* - "A" declares "B" as hostile: Shows "A" and "B" both as hostiles.
* - "A" declares "B" as neutral: Shows "A" and "B" both as neutrals, except "B" declared "A" as hostile. Degrades "B"'s allied status to neutral towards "A". 
* - "A" declares "B" as allied:  "A" offers "B" friend invite. If "A" and "B" both agree on allied state, upgrades both status to allied.
* - "A" shoots a member of a group with more than 1 members. Declares everyone hostile towards "A".
* - Allied players or group members will never turn each other hostile by damaging.
* 
*/

namespace IFF
{
    const std::string HOSTILE_VALUE = "hostile";
    const std::string ALLIED_VALUE = "allied";

    enum class Attitude
    {
        Neutral,
        Hostile,
        Allied
    };

    static std::unordered_map<std::string, std::unordered_map<std::string, Attitude>> characterFileNamesToCharacterFileNameAttitudes;

    void ApplyAttitude(const std::pair<uint, uint>& clientIds, const Attitude attitude)
    {
        float attitudeValue;
        switch (attitude)
        {
            case Attitude::Hostile:
                attitudeValue = -1.0f;
                break;

            case Attitude::Allied:
                attitudeValue = 1.0f;
                break;

            default:
                attitudeValue = 0.0f;
                break;
        }

        std::pair<int, int> reputationMapIds;
        pub::Player::GetRep(clientIds.first, reputationMapIds.first);
        pub::Player::GetRep(clientIds.second, reputationMapIds.second);

        pub::Reputation::SetAttitude(reputationMapIds.first, reputationMapIds.second, attitudeValue);
        pub::Reputation::SetAttitude(reputationMapIds.second, reputationMapIds.first, attitudeValue);
    }

    std::string GetCharacterFileName(const std::wstring& characterName)
    {
        std::wstring characterFileName;
        if (HkGetCharFileName(characterName, characterFileName) != HKE_OK)
            return "";
        return wstos(characterFileName);
    }

    std::pair<Attitude, Attitude> GetAttitudeTowards(const std::pair<std::wstring, std::wstring>& characterNames)
    {
        const std::string& firstCharacterFileName = GetCharacterFileName(characterNames.first);
        const std::string& secondCharacterFileName = GetCharacterFileName(characterNames.second);
        return {
            characterFileNamesToCharacterFileNameAttitudes[firstCharacterFileName].contains(secondCharacterFileName) ? characterFileNamesToCharacterFileNameAttitudes[firstCharacterFileName][secondCharacterFileName] : Attitude::Neutral,
            characterFileNamesToCharacterFileNameAttitudes[secondCharacterFileName].contains(firstCharacterFileName) ? characterFileNamesToCharacterFileNameAttitudes[secondCharacterFileName][firstCharacterFileName] : Attitude::Neutral
        };
    }

    std::wstring GetCharacterName(const uint clientId)
    {
        return (wchar_t*)Players.GetActiveCharacterName(clientId);
    }

    void UpdateAttitude(const uint clientId, const uint otherClientId)
    {
        const auto& attitudes = GetAttitudeTowards({ GetCharacterName(clientId), GetCharacterName(otherClientId) });
        Attitude priorityAttitude = Attitude::Neutral;
        if (attitudes.first == attitudes.second)
        {
            priorityAttitude = attitudes.first;
        }
        else if (attitudes.first == Attitude::Hostile || attitudes.second == Attitude::Hostile)
        {
            priorityAttitude = Attitude::Hostile;
        }
        else if (attitudes.first == Attitude::Neutral || attitudes.second == Attitude::Neutral)
        {
            priorityAttitude = Attitude::Neutral;
        }

        ApplyAttitude({ clientId, otherClientId }, priorityAttitude);
    }

    std::string GetPlayerAttitudesFilePath()
    {
        return scAcctPath + "\\iff.ini";
    }

    static bool initialized = false;
    void ReadCharacterData()
    {
        if (initialized)
            return;
        initialized = true;
        INI_Reader ini;
        if (ini.open(GetPlayerAttitudesFilePath().c_str(), false))
        {
            while (ini.read_header())
            {
                const std::string characterFileName = std::string(ini.get_header_ptr());
                while (ini.read_value())
                {
                    const std::string& attitudeValue = ini.get_value_string(0);
                    Attitude attitude;
                    if (attitudeValue == HOSTILE_VALUE)
                        attitude = Attitude::Hostile;
                    else if (attitudeValue == ALLIED_VALUE)
                        attitude = Attitude::Allied;
                    else
                        attitude = Attitude::Neutral;
                    characterFileNamesToCharacterFileNameAttitudes[characterFileName][Trim(std::string(ini.get_line_ptr())).substr(0, 11)] = attitude;
                }
            }
            ini.close();
        }
    }

    std::string GetAttitudeStringValue(const Attitude attitude)
    {
        std::string attitudeValue;
        switch (attitude)
        {
        case Attitude::Hostile:
            return HOSTILE_VALUE;

        case Attitude::Allied:
            return ALLIED_VALUE;
        }
        return "";
    }

    void WriteCharacterAttitude(const std::wstring& sourceCharacterName, const std::wstring& targetCharacterName, const Attitude attitude)
    {
        const std::string& sourceCharacterFileName = GetCharacterFileName(sourceCharacterName);
        const std::string& targetCharacterFileName = GetCharacterFileName(targetCharacterName);
        characterFileNamesToCharacterFileNameAttitudes[sourceCharacterFileName][targetCharacterFileName] = attitude;

        if (attitude != Attitude::Neutral)
            IniWrite(GetPlayerAttitudesFilePath(), sourceCharacterFileName, targetCharacterFileName, GetAttitudeStringValue(attitude));
        else
            IniDelete(GetPlayerAttitudesFilePath(), sourceCharacterFileName, targetCharacterFileName);
    }

    uint GetClientId(const std::wstring& characterName)
    {
        uint clientId = -1;
        bool idStringType = false;
        if (HkGetClientID(idStringType, clientId, characterName) == HKE_OK)
            return clientId;
        return -1;
    }

    std::wstring GetCharacterNameByTarget(const uint targettingClientId)
    {
        uint shipId;
        pub::Player::GetShip(targettingClientId, shipId);
        if (!shipId)
            return L"";

        uint targetId;
        pub::SpaceObj::GetTarget(shipId, targetId);
        if (!targetId)
            return L"";

        const uint targetClientId = HkGetClientIDByShip(targetId);
        if (targetClientId)
            return GetCharacterName(targetClientId);

        return L"";
    }

    std::pair<Attitude, Attitude> TrySetAttitudeTowardsTarget(const uint currentClientId, const std::wstring targetCharacterName, const Attitude attitude)
    {
        std::pair<Attitude, Attitude> attitudeChange = { attitude, attitude };

        if (targetCharacterName.empty())
        {
            PrintUserCmdText(currentClientId, L"Select or type a character name to change IFF towards.");
            return attitudeChange;
        }

        const uint targetClientId = GetClientId(targetCharacterName);
        if (!HkIsValidClientID(targetClientId))
        {
            PrintUserCmdText(currentClientId, L"Character '" + targetCharacterName + L"' is not logged in.");
            return attitudeChange;
        }

        const std::wstring currentCharacterName = GetCharacterName(currentClientId);
        if (currentCharacterName == targetCharacterName)
            return attitudeChange;

        const auto& attitudes = GetAttitudeTowards({ currentCharacterName, targetCharacterName });
        attitudeChange.first = attitudes.first;
        if (attitudeChange.first == attitudeChange.second)
            return attitudeChange;

        WriteCharacterAttitude(currentCharacterName, targetCharacterName, attitude);
        UpdateAttitude(currentClientId, targetClientId);

        return attitudeChange;
    }

    void UserCmd_Hostile(const uint clientId, const std::wstring& arguments)
    {
        std::wstring targetCharacterName = Trim(GetParamToEnd(arguments, ' ', 0));
        if (targetCharacterName.empty())
            targetCharacterName = GetCharacterNameByTarget(clientId);
        const auto& attitudeChange = TrySetAttitudeTowardsTarget(clientId, targetCharacterName, Attitude::Hostile);
        if (attitudeChange.first != attitudeChange.second)
        {
            PrintUserCmdText(clientId, L"You declared hostility towards " + targetCharacterName);
            const uint targetClientId = GetClientId(targetCharacterName);
            PrintUserCmdText(targetClientId, GetCharacterName(clientId) + L" declared hostility.");
        }
    }

    void UserCmd_Neutral(const uint clientId, const std::wstring& arguments)
    {
        std::wstring targetCharacterName = Trim(GetParamToEnd(arguments, ' ', 0));
        if (targetCharacterName.empty())
            targetCharacterName = GetCharacterNameByTarget(clientId);
        const auto& attitudeChange = TrySetAttitudeTowardsTarget(clientId, targetCharacterName, Attitude::Neutral);
        if (attitudeChange.first != attitudeChange.second)
        {
            const uint targetClientId = GetClientId(targetCharacterName);
            const std::wstring& currentCharacterName = GetCharacterName(clientId);
            if (attitudeChange.first == Attitude::Hostile)
            {
                PrintUserCmdText(clientId, L"You gave up hostility towards " + targetCharacterName);
                PrintUserCmdText(targetClientId, currentCharacterName + L" terminated hostility.");
            }
            else
            {
                PrintUserCmdText(clientId, L"You terminated friendship towards " + targetCharacterName);
                PrintUserCmdText(targetClientId, currentCharacterName + L" terminated friendship.");
            }
        }
    }

    void UserCmd_Allied(const uint clientId, const std::wstring& arguments)
    {
        std::wstring targetCharacterName = Trim(GetParamToEnd(arguments, ' ', 0));
        if (targetCharacterName.empty())
            targetCharacterName = GetCharacterNameByTarget(clientId);
        const auto& attitudeChange = TrySetAttitudeTowardsTarget(clientId, targetCharacterName, Attitude::Allied);
        if (attitudeChange.first != attitudeChange.second)
        {
            const uint targetClientId = GetClientId(targetCharacterName);
            const std::wstring& currentCharacterName = GetCharacterName(clientId);
            const auto currentAttitude = GetAttitudeTowards({ currentCharacterName, targetCharacterName });
            if (currentAttitude.second != Attitude::Allied)
            {
                PrintUserCmdText(clientId, L"You offered friendship towards " + targetCharacterName);
                PrintUserCmdText(targetClientId, currentCharacterName + L" offered you friendship. Type '/friend " + currentCharacterName + L"' to accept.");
            }
            else if (currentAttitude.first == currentAttitude.second)
            {
                PrintUserCmdText(clientId, targetCharacterName + L" is now your friend.");
                PrintUserCmdText(targetClientId, currentCharacterName + L" is now your friend.");
            }
        }
    }

    std::wstring GetAttitudeDisplayString(const Attitude attitude)
    {
        switch (attitude)
        {
        case Attitude::Hostile:
            return L"hostile";

        case Attitude::Neutral:
            return L"neutral";

        case Attitude::Allied:
            return L"friendly";
        }
    }

    void UserCmd_Attitude(const uint clientId, const std::wstring& arguments)
    {
        std::wstring targetCharacterName = Trim(GetParamToEnd(arguments, ' ', 0));
        if (targetCharacterName.empty())
            targetCharacterName = GetCharacterNameByTarget(clientId);

        if (targetCharacterName.empty())
        {
            PrintUserCmdText(clientId, L"Select or type a character name to request IFF about.");
            return;
        }

        if (HkIsValidClientID(GetClientId(targetCharacterName)))
        {
            const auto& attitude = GetAttitudeTowards({ GetCharacterName(clientId), targetCharacterName });
            PrintUserCmdText(clientId, L"You are " + GetAttitudeDisplayString(attitude.first) + L" towards " + targetCharacterName + L", and they are " + GetAttitudeDisplayString(attitude.second) + L" towards you.");
        }
        else
            PrintUserCmdText(clientId, L"Character '" + targetCharacterName + L"' is not logged in.");
    }

    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship)
    {
        returncode = DEFAULT_RETURNCODE;

        if (clientId && ship.iClientID)
            UpdateAttitude(clientId, ship.iClientID);

        return true;
    }

    bool AreInSameGroup(const uint clientAId, const uint clientBId)
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

    void __stdcall HkCb_AddDmgEntry(DamageList* damageList, unsigned short subObjectId, float hitpoints, DamageEntry::SubObjFate fate)
    {
        returncode = DEFAULT_RETURNCODE;

        if (!damageList->is_inflictor_a_player() || damageList->get_cause() == 0x01 || !HkIsValidClientID(iDmgTo))
            return;

        const uint damagerClientId = HkGetClientIDByShip(damageList->get_inflictor_id());
        if (!damagerClientId)
            return;

        if (AreInSameGroup(damagerClientId, iDmgTo))
            return;

        // No-PvP system check
        uint damagerSystemId;
        pub::Player::GetSystem(damagerClientId, damagerSystemId);
        for (const auto& noPvPSystem : map_mapNoPVPSystems)
            if (noPvPSystem.second == damagerSystemId)
                return;

        // Check if both players are in same system - reducing false positives due to FLHook bugged damage detection implementation
        uint damagedSystemId;
        pub::Player::GetSystem(iDmgTo, damagedSystemId);
        if (!damagerSystemId || damagerSystemId != damagedSystemId)
            return;

        // Check if both players are in a range - limiting same bug as above
        uint damagerShipId;
        pub::Player::GetShip(damagerClientId, damagerShipId);
        if (!damagerShipId)
            return;
        uint damagedShipId;
        pub::Player::GetShip(iDmgTo, damagedShipId);
        if (!damagedShipId)
            return;
        Vector damagerShipVector;
        Matrix damagerShipRotation;
        pub::SpaceObj::GetLocation(damagerShipId, damagerShipVector, damagerShipRotation);
        Vector damagedShipVector;
        Matrix damagedShipRotation;
        pub::SpaceObj::GetLocation(damagedShipId, damagedShipVector, damagedShipRotation);
        if (HkDistance3D(damagerShipVector, damagedShipVector) > 20000.0f)
            return;

        const auto& damageInflictorCharacterName = GetCharacterName(damagerClientId);

        std::list<GROUP_MEMBER> members;
        if (HkGetGroupMembers(ARG_CLIENTID(iDmgTo), members) != HKE_OK || members.size() < 1)
        {
            GROUP_MEMBER member;
            member.iClientID = iDmgTo;
            member.wscCharname = GetCharacterName(iDmgTo);
            members.push_back(member);
        }

        for (const GROUP_MEMBER& member : members)
        {
            const auto& lastAttitude = GetAttitudeTowards({ member.wscCharname, damageInflictorCharacterName });

            // Players which are allied will never get hostile to each other by shooting. Except they are organized in a group. The group integrity has priority and will always cancel any friendships.
            if (members.size() < 2 && (lastAttitude.first == Attitude::Allied && lastAttitude.second == Attitude::Allied))
                continue;

            const auto& attitudeChange = TrySetAttitudeTowardsTarget(member.iClientID, damageInflictorCharacterName, Attitude::Hostile);
            if (attitudeChange.first != attitudeChange.second && lastAttitude.second != Attitude::Hostile)
            {
                PrintUserCmdText(member.iClientID, damageInflictorCharacterName + L" attacked!");
                if (lastAttitude.first == lastAttitude.second && lastAttitude.second == Attitude::Allied)
                {
                    PrintUserCmdText(member.iClientID, damageInflictorCharacterName + L" terminated friendship.");
                    PrintUserCmdText(damagerClientId, member.wscCharname + L" terminated friendship.");
                }
            }
        }
    }

    void DeleteCharacterFromIFF(const std::string& characterFileName)
    {
        characterFileNamesToCharacterFileNameAttitudes.erase(characterFileName);
        IniDelSection(GetPlayerAttitudesFilePath(), characterFileName);

        for (auto& characterFileNameToCharacterFileNameAttitudes : characterFileNamesToCharacterFileNameAttitudes)
        {
            if (characterFileNameToCharacterFileNameAttitudes.second.contains(characterFileName))
            {
                characterFileNameToCharacterFileNameAttitudes.second.erase(characterFileName);
                IniDelete(GetPlayerAttitudesFilePath(), characterFileNameToCharacterFileNameAttitudes.first, characterFileName);
                continue;
            }
        }
    }

    void __stdcall CreateNewCharacter_After(SCreateCharacterInfo const& info, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;
        DeleteCharacterFromIFF(GetCharacterFileName(info.wszCharname));
    }

    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId)
    {
        returncode = DEFAULT_RETURNCODE;
        const std::string characterFileName = std::string(characterId.szCharFilename).substr(0, 11);
        DeleteCharacterFromIFF(characterFileName);
    }

    static std::string characterFileNameToRename;

    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        returncode = DEFAULT_RETURNCODE;
        if (onlyDelete)
            characterFileNameToRename = "";
        else
            characterFileNameToRename = GetCharacterFileName(charname);
        return HKE_OK;
    }

    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        returncode = DEFAULT_RETURNCODE;
        if (!characterFileNameToRename.empty())
        {
            const std::string& characterFileName = GetCharacterFileName(newCharname);
            for (auto& characterFileNameToCharacterFileNameAttitudes : characterFileNamesToCharacterFileNameAttitudes)
            {
                if (characterFileNameToCharacterFileNameAttitudes.second.contains(characterFileNameToRename))
                {
                    characterFileNameToCharacterFileNameAttitudes.second[characterFileName] = characterFileNameToCharacterFileNameAttitudes.second[characterFileNameToRename];
                    characterFileNameToCharacterFileNameAttitudes.second.erase(characterFileNameToRename);
                    const std::string& attitudeValue = GetAttitudeStringValue(characterFileNameToCharacterFileNameAttitudes.second[characterFileName]);
                    if (!attitudeValue.empty())
                        IniWrite(GetPlayerAttitudesFilePath(), characterFileNameToCharacterFileNameAttitudes.first, characterFileName, attitudeValue);
                    IniDelete(GetPlayerAttitudesFilePath(), characterFileNameToCharacterFileNameAttitudes.first, characterFileNameToRename);
                    continue;
                }
            }

            characterFileNamesToCharacterFileNameAttitudes[characterFileName] = characterFileNamesToCharacterFileNameAttitudes[characterFileNameToRename];
            characterFileNamesToCharacterFileNameAttitudes.erase(characterFileNameToRename);
            IniDelSection(GetPlayerAttitudesFilePath(), characterFileNameToRename);
            for (const auto& characterFileNameAttitudes : characterFileNamesToCharacterFileNameAttitudes[characterFileName])
            {
                const std::string& attitudeValue = GetAttitudeStringValue(characterFileNameAttitudes.second);
                if (!attitudeValue.empty())
                    IniWrite(GetPlayerAttitudesFilePath(), characterFileName, characterFileNameAttitudes.first, attitudeValue);
            }
        }
        characterFileNameToRename = "";
        return HKE_OK;
    }
}
