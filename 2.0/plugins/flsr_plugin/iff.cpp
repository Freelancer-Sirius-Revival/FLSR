#include "main.h"

/**
* IFF PLUGIN - setting players hostile, neutral, or allied in space.
* by Skotty
* 
* Cases:
* - "A" declares "B" as hostile: Shows "A" and "B" both as hostiles.
* - "A" declares "B" as neutral: Shows "A" and "B" both as neutrals, except "B" declared "A" as hostile. Degrades "B"'s allied status to neutral towards "A". 
* - "A" declares "B" as allied:  Shows "A" and "B" both as allies,   except "B" declared "A" as hostile. Upgrades "B"'s neutral status to allied towards "A".
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

    void UpdateAttitude(const uint clientId, const uint otherClientId)
    {
        const auto& attitudes = GetAttitudeTowards({ (wchar_t*)Players.GetActiveCharacterName(clientId), (wchar_t*)Players.GetActiveCharacterName(otherClientId) });
        Attitude priorityAttitude;
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
        else
        {
            priorityAttitude = Attitude::Allied;
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

    bool DoesCharacterExist(const std::wstring& characterName)
    {
        st6::wstring str((ushort*)characterName.c_str());
        return Players.FindAccountFromCharacterName(str);
    }

    uint GetClientId(const std::wstring& characterName)
    {
        uint clientId = -1;
        bool idStringType = false;
        HkGetClientID(idStringType, clientId, characterName);
        return clientId;
    }

    void TrySetAttitudeTowardsTarget(const uint currentClientId, const std::wstring targetCharacterName, const Attitude attitude)
    {
        if (!DoesCharacterExist(targetCharacterName))
        {
            PrintUserCmdText(currentClientId, L"Character '" + targetCharacterName + L"' does not exist.");
            return;
        }

        const std::wstring currentCharacterName = (wchar_t*)Players.GetActiveCharacterName(currentClientId);
        if (currentCharacterName == targetCharacterName)
            return;

        const auto& attitudes = GetAttitudeTowards({ currentCharacterName, targetCharacterName });
        if (attitudes.first == attitude)
            return;

        WriteCharacterAttitude(currentCharacterName, targetCharacterName, attitude);
        if (attitude != Attitude::Hostile)
        {
            if (attitudes.second != Attitude::Hostile)
                WriteCharacterAttitude(targetCharacterName, currentCharacterName, attitude);
            else
                PrintUserCmdText(currentClientId, targetCharacterName + L" must end the hostility to allow IFF changes.");
        }
        else
            PrintUserCmdText(currentClientId, L"You are now permanently hostile with " + targetCharacterName);

        const uint targetClientId = GetClientId(targetCharacterName);
        if (HkIsValidClientID(targetClientId))
            UpdateAttitude(currentClientId, targetClientId);
    }

    void UserCmd_Hostile(const uint clientId, const std::wstring& arguments)
    {
        TrySetAttitudeTowardsTarget(clientId, Trim(GetParamToEnd(arguments, ' ', 0)), Attitude::Hostile);
    }

    void UserCmd_Neutral(const uint clientId, const std::wstring& arguments)
    {
        TrySetAttitudeTowardsTarget(clientId, Trim(GetParamToEnd(arguments, ' ', 0)), Attitude::Neutral);
    }

    void UserCmd_Allied(const uint clientId, const std::wstring& arguments)
    {
        TrySetAttitudeTowardsTarget(clientId, Trim(GetParamToEnd(arguments, ' ', 0)), Attitude::Allied);
    }

    bool Send_FLPACKET_SERVER_CREATESHIP_AFTER(uint clientId, FLPACKET_CREATESHIP& ship)
    {
        returncode = DEFAULT_RETURNCODE;

        if (clientId && ship.iClientID)
            UpdateAttitude(clientId, ship.iClientID);

        return true;
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
