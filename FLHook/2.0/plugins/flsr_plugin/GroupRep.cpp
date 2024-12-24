#include "main.h"

namespace GroupReputation
{
    const float MAX_REPUTATION_TRANSFER_DISTANCE = 10000.0f;

    struct RepChangeEffects
    {
        uint groupId = 0;
        float objectDestruction = 0;
        std::vector<std::pair<uint, float>> empathyRates;
    };

    static std::unordered_map<uint, RepChangeEffects> groupReputationChangeEffects;

    static float maxReputationThreshold = 0.9f;

    void LoadEmpathy()
    {
        std::string dataPath;
        INI_Reader ini;
        if (ini.open("freelancer.ini", false))
        {
            while (ini.read_header() && dataPath.empty())
            {
                if (ini.is_header("Freelancer"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("data path"))
                        {
                            dataPath = ini.get_value_string(0);
                            break;
                        }
                    }
                }
            }
            ini.close();
        }

        if (!dataPath.empty() && ini.open((dataPath + "\\MISSIONS\\empathy.ini").c_str(), false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("RepChangeEffects"))
                {
                    RepChangeEffects repChange;
                    while (ini.read_value())
                    {
                        if (ini.is_value("group"))
                        {
                            uint groupId;
                            pub::Reputation::GetReputationGroup(groupId, ini.get_value_string(0));
                            repChange.groupId = groupId;
                        }

                        if (ini.is_value("event") && strcmpi(ini.get_value_string(0), "object_destruction") == 0)
                            repChange.objectDestruction = ini.get_value_float(1);

                        if (ini.is_value("empathy_rate"))
                        {
                            uint groupId;
                            pub::Reputation::GetReputationGroup(groupId, ini.get_value_string(0));
                            repChange.empathyRates.push_back({ groupId, ini.get_value_float(1) });
                        }
                    }
                    if (repChange.groupId)
                        groupReputationChangeEffects[repChange.groupId] = repChange;
                }
            }
            ini.close();
        }
    }

    HMODULE LoadContentDll()
    {
        INI_Reader ini;
        if (ini.open("freelancer.ini", false))
        {
            while (ini.read_header())
            {
                if (ini.is_header("Initial MP DLLs"))
                {
                    while (ini.read_value())
                    {
                        if (ini.is_value("path"))
                        {
                            const std::string dataPath = ini.get_value_string(0);
                            return GetModuleHandle((dataPath + "\\Content.dll").c_str());
                        }
                    }
                }
            }
            ini.close();
        }
        return 0;
    }

    static bool initialized = false;

    void InitializeWithGameData()
    {
        if (initialized)
            return;
        initialized = true;

        LoadEmpathy();

        const HMODULE contentHandle = LoadContentDll();
        if (contentHandle)
            maxReputationThreshold = 1.0f - *(double*)(DWORD(contentHandle) + 0x11B930);
    }

    void ChangeReputationRelative(const int clientReputationId, const int groupId, const float value)
    {
        if (value == 0.0f)
            return;

        float currentValue;
        pub::Reputation::GetGroupFeelingsTowards(clientReputationId, groupId, currentValue);
        pub::Reputation::SetReputation(clientReputationId, groupId, std::max(-maxReputationThreshold, std::min(currentValue + value, maxReputationThreshold)));
    }

    void ChangeReputationOfGroup(const uint clientId, const uint otherGroupId)
    {
        if (!HkIsValidClientID(clientId))
            return;

        if (!groupReputationChangeEffects.contains(otherGroupId))
            return;

        uint clientShipId;
        pub::Player::GetShip(clientId, clientShipId);
        if (!clientShipId)
            return;

        uint clientSystemId;
        pub::Player::GetSystem(clientId, clientSystemId);
        if (!clientSystemId)
            return;

        int clientReputationId;
        pub::Player::GetRep(clientId, clientReputationId);

        std::list<GROUP_MEMBER> members;
        if (HkGetGroupMembers(ARG_CLIENTID(clientId), members) != HKE_OK)
            return;

        Vector clientShipVector;
        Matrix clientShipRotation;
        pub::SpaceObj::GetLocation(clientShipId, clientShipVector, clientShipRotation);

        for (const auto& member : members)
        {
            if (member.iClientID != clientId)
            {
                uint memberSystemId;
                pub::Player::GetSystem(member.iClientID, memberSystemId);
                if (memberSystemId != clientSystemId)
                    continue;

                uint memberShipId;
                pub::Player::GetShip(member.iClientID, memberShipId);
                if (!memberShipId)
                    continue;

                Vector memberShipVector;
                Matrix memberShipRotation;
                pub::SpaceObj::GetLocation(memberShipId, memberShipVector, memberShipRotation);

                if (HkDistance3D(clientShipVector, memberShipVector) > MAX_REPUTATION_TRANSFER_DISTANCE)
                    continue;

                int memberReputationId;
                pub::Player::GetRep(member.iClientID, memberReputationId);
                const auto& reputationChangeEffects = groupReputationChangeEffects[otherGroupId];

                // Add direct reputation change of the destroyed object.
                ChangeReputationRelative(memberReputationId, otherGroupId, reputationChangeEffects.objectDestruction);

                // Add indirect reputation changes of the destroyed object.
                for (const auto& empathyRate : reputationChangeEffects.empathyRates)
                    ChangeReputationRelative(memberReputationId, empathyRate.first, reputationChangeEffects.objectDestruction * empathyRate.second);
            }
        }
    }

    void __stdcall SolarDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (!killed)
            return;

        const uint killerClientId = HkGetClientIDByShip(killerShipId);
        if (!killerClientId)
            return;

        // Solars have their nickname ID directly mapped to their Reputation ID
        const uint victimReputationId = (killedObject->cobj)->get_id();
        uint victimGroupId;
        Reputation::Vibe::GetAffiliation(victimReputationId, victimGroupId, false);

        ChangeReputationOfGroup(killerClientId, victimGroupId);
    }

    void __stdcall ShipDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
    {
        returncode = DEFAULT_RETURNCODE;

        if (!killed || killedObject->cobj->GetOwnerPlayer())
            return;

        const uint killerClientId = HkGetClientIDByShip(killerShipId);
        if (!killerClientId)
            return;

        const uint victimReputationId = reinterpret_cast<CShip*>(killedObject->cobj)->repVibe;
        uint victimGroupId;
        Reputation::Vibe::GetAffiliation(victimReputationId, victimGroupId, false);

        ChangeReputationOfGroup(killerClientId, victimGroupId);
    }
}