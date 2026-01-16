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
        const std::wstring& currentCharacterName = GetCharacterName(clientId);
        const auto& lastAttitude = GetAttitudeTowards({ currentCharacterName, targetCharacterName });
        const auto& attitudeChange = TrySetAttitudeTowardsTarget(clientId, targetCharacterName, Attitude::Neutral);
        if (attitudeChange.first == attitudeChange.second)
            return;

        const uint targetClientId = GetClientId(targetCharacterName);
        if (attitudeChange.first == Attitude::Hostile)
        {
            PrintUserCmdText(clientId, L"You gave up hostility towards " + targetCharacterName);
            PrintUserCmdText(targetClientId, currentCharacterName + L" terminated hostility.");
        }
        else if (lastAttitude.first == lastAttitude.second && lastAttitude.first == Attitude::Allied)
        {
            PrintUserCmdText(clientId, L"You terminated friendship towards " + targetCharacterName);
            PrintUserCmdText(targetClientId, currentCharacterName + L" terminated friendship.");
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

            default:
                return L"";
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
        if (clientId && ship.clientId)
            UpdateAttitude(clientId, ship.clientId);

        returncode = DEFAULT_RETURNCODE;
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

    void ShipDamaged(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList)
    {
        if (incomingDamage <= 0.0f)
            return;

        const uint damagedClientId = damagedObject->cobj->GetOwnerPlayer();
        if (!damageList->is_inflictor_a_player() || damageList->get_cause() == DamageCause::Collision || !HkIsValidClientID(damagedClientId))
            return;

        const uint inflictorClientId = damageList->get_inflictor_owner_player();
        if (!HkIsValidClientID(inflictorClientId))
            return;

        if (AreInSameGroup(inflictorClientId, damagedClientId))
            return;

        // No-PvP system check
        uint damagedSystemId;
        pub::Player::GetSystem(damagedClientId, damagedSystemId);
        for (const auto& noPvPSystem : map_mapNoPVPSystems)
        {
            if (noPvPSystem.second == damagedSystemId)
                return;
        }

        const std::wstring& inflictorCharacterName = GetCharacterName(inflictorClientId);
        const std::wstring& damagedCharacterName = GetCharacterName(damagedClientId);

        std::list<GROUP_MEMBER> members;
        // Players always should be in a group - even if just their own single-person group.
        if (HkGetGroupMembers(ARG_CLIENTID(damagedClientId), members) != HKE_OK)
            return;

        if (members.size() > 1)
        {
            for (const GROUP_MEMBER& member : members)
            {
                const auto& lastAttitude = GetAttitudeTowards({ member.wscCharname, inflictorCharacterName });
                const auto& attitudeChange = TrySetAttitudeTowardsTarget(member.iClientID, inflictorCharacterName, Attitude::Hostile);
                if (attitudeChange.first != attitudeChange.second && lastAttitude.second != Attitude::Hostile)
                {
                    std::wstring attackText = inflictorCharacterName + L" attacked your group";
                    if (lastAttitude.first == lastAttitude.second && lastAttitude.second == Attitude::Allied)
                    {
                        attackText = attackText + L" and terminated friendship";
                        PrintUserCmdText(inflictorClientId, member.wscCharname + L" terminated friendship, because you attacked their group.");
                    }
                    PrintUserCmdText(member.iClientID, attackText + L"!");
                }
            }
        }
        else
        {
            const auto& lastAttitude = GetAttitudeTowards({ damagedCharacterName, inflictorCharacterName });
            // If they are friends, ignore any shots fired.
            if (lastAttitude.first == Attitude::Allied && lastAttitude.second == Attitude::Allied)
                return;

            const auto& attitudeChange = TrySetAttitudeTowardsTarget(damagedClientId, inflictorCharacterName, Attitude::Hostile);
            if (attitudeChange.first != attitudeChange.second && lastAttitude.second != Attitude::Hostile)
                PrintUserCmdText(damagedClientId, inflictorCharacterName + L" attacked!");
        }
    }

    void __stdcall ShipEquipDamage(const IObjRW* damagedObject, const CEquip* hitEquip, const float& incomingDamage, const DamageList* damageList)
    {
        ShipDamaged(damagedObject, incomingDamage, damageList);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall ShipShieldDamage(const IObjRW* damagedObject, const CEShield* hitShield, const float& incomingDamage, const DamageList* damageList)
    {
        ShipDamaged(damagedObject, incomingDamage, damageList);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall ShipColGrpDamage(const IObjRW* damagedObject, const CArchGroup* hitColGrp, const float& incomingDamage, const DamageList* damageList)
    {
        ShipDamaged(damagedObject, incomingDamage, damageList);
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall ShipHullDamage(const IObjRW* damagedObject, const float& incomingDamage, const DamageList* damageList)
    {
        ShipDamaged(damagedObject, incomingDamage, damageList);
        returncode = DEFAULT_RETURNCODE;
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
        DeleteCharacterFromIFF(GetCharacterFileName(info.wszCharname));
        returncode = DEFAULT_RETURNCODE;
    }

    void __stdcall DestroyCharacter_After(CHARACTER_ID const& characterId, unsigned int clientId)
    {
        const std::string characterFileName = std::string(characterId.charFilename).substr(0, 11);
        DeleteCharacterFromIFF(characterFileName);
        returncode = DEFAULT_RETURNCODE;
    }

    static std::string characterFileNameToRename;

    HK_ERROR HkRename(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
        if (onlyDelete)
            characterFileNameToRename = "";
        else
            characterFileNameToRename = GetCharacterFileName(charname);
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }

    HK_ERROR HkRename_After(const std::wstring& charname, const std::wstring& newCharname, bool onlyDelete)
    {
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
        returncode = DEFAULT_RETURNCODE;
        return HKE_OK;
    }
}
