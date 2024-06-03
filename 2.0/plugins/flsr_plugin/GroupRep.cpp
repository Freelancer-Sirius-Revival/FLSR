#include "main.h"

namespace GroupReputation
{
    struct RepChangeEffects
    {
        uint groupId = 0;
        float objectDestruction = 0;
        std::vector<std::pair<uint, float>> empathyRates;
    };

    static std::unordered_map<uint, RepChangeEffects> groupReputationChangeEffects;

    static std::unordered_map<uint, mstime> destroyedShipIdsWithTimeStamp;

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

        const auto contentHandle = LoadContentDll;
        if (contentHandle)
            maxReputationThreshold = 1.0f - *(double*)(DWORD(contentHandle) + 0x11B930);
    }

    void ChangeReputationRelative(int clientReputationId, int groupId, float value)
    {
        if (value == 0.0f)
            return;

        float currentValue;
        pub::Reputation::GetGroupFeelingsTowards(clientReputationId, groupId, currentValue);
        pub::Reputation::SetReputation(clientReputationId, groupId, std::max(-maxReputationThreshold, std::min(currentValue + value, maxReputationThreshold)));
    }

    void ChangeReputationOfGroup(uint clientId, uint otherGroupId)
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

                if (HkDistance3D(clientShipVector, memberShipVector) > 10000.0f)
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

    void BaseDestroyed(uint objectId, uint clientId)
    {
        int baseReputationId;
        pub::SpaceObj::GetSolarRep(objectId, baseReputationId);
        if (!baseReputationId)
            return;

        uint baseGroupId;
        Reputation::Vibe::GetAffiliation(baseReputationId, baseGroupId, false);
        if (!baseGroupId)
            return;

        ChangeReputationOfGroup(clientId, baseGroupId);
    }

    void __stdcall ShipDestroyed(DamageList* dmg, DWORD* ecx, uint killed)
    {
        returncode = DEFAULT_RETURNCODE;

        if (!killed)
            return;
        CShip* cship = (CShip*)ecx[4];
        if (cship->GetOwnerPlayer())
            return;
        uint victimShipId = cship->iID;
        if (!victimShipId)
            return;

        if (destroyedShipIdsWithTimeStamp.contains(victimShipId))
            return;

        destroyedShipIdsWithTimeStamp[victimShipId] = timeInMS();

        //unknown and mutating(??) datatype. Casted to FLPACKET_UNKNOWN for ease of access to individual elements
        FLPACKET_UNKNOWN* data = reinterpret_cast<FLPACKET_UNKNOWN*>(dmg);

        //First argument is some unknown datatype pointer, not DamageList
        //In case of ship having died to a death fuse, [2] is equal to 1 and actual killer spaceObjId is in [5]
        //When death fuse has a duration > 0, both events fire, in case of an instant death fuse, only the fuse death is broadcasted.
        //As such, we need to handle all 3 scenarios (no fuse, long fuse, instant fuse) by checking for both events and avoiding paying out the bounty twice.

        uint killerShipId;
        if (data->iDunno[4]) // Represents death's damage cause in case of fuse death
        {
            killerShipId = data->iDunno[5]; // in case of fuse death, killerShipId is held here
        }
        else
        {
            killerShipId = data->iDunno[2]; // in case of fuse death equals an indetermined non-zero value, otherwise it is killerShipId.
        }
        uint killerClientId = HkGetClientIDByShip(killerShipId);

        int victimReputationId;
        pub::SpaceObj::GetRep(victimShipId, victimReputationId);
        uint victimGroupId;
        Reputation::Vibe::GetAffiliation(victimReputationId, victimGroupId, false);

        ChangeReputationOfGroup(killerClientId, victimGroupId);
    }

    void CleanDestroyedShipRegistry()
    {
        const mstime now = timeInMS();
        std::vector<uint> shipIdsToRemove;
        for (const auto& shipIdTimeStamp : destroyedShipIdsWithTimeStamp)
        {
            if (shipIdTimeStamp.second + 30000 < now)
                shipIdsToRemove.push_back(shipIdTimeStamp.first);
        }

        for (const uint shipId : shipIdsToRemove)
            destroyedShipIdsWithTimeStamp.erase(shipId);
    }
}