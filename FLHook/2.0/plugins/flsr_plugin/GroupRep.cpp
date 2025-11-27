#include "GroupRep.h"
#include "Empathies.h"
#include "Plugin.h"

namespace GroupReputation
{
    const float MAX_REPUTATION_TRANSFER_DISTANCE = 10000.0f;

    static void ChangeReputationOfGroup(const uint clientId, const uint otherGroupId)
    {
        if (!HkIsValidClientID(clientId))
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

        st6::vector<unsigned int> members;
        pub::Player::GetGroupMembers(clientId, members);

        Vector clientShipVector;
        Matrix clientShipRotation;
        pub::SpaceObj::GetLocation(clientShipId, clientShipVector, clientShipRotation);

        for (const auto& memberClientId : members)
        {
            if (memberClientId == clientId)
                continue;

            uint memberSystemId;
            pub::Player::GetSystem(memberClientId, memberSystemId);
            if (memberSystemId != clientSystemId)
                continue;

            uint memberShipId;
            pub::Player::GetShip(memberClientId, memberShipId);
            if (!memberShipId)
                continue;

            Vector memberShipVector;
            Matrix memberShipRotation;
            pub::SpaceObj::GetLocation(memberShipId, memberShipVector, memberShipRotation);

            if (HkDistance3D(clientShipVector, memberShipVector) > MAX_REPUTATION_TRANSFER_DISTANCE)
                continue;

            Empathies::ChangeReputationsByReason(memberClientId, otherGroupId, Empathies::ReputationChangeReason::ObjectDestruction);
        }
    }

    void __stdcall ObjectDestroyed(const IObjRW* killedObject, const bool killed, const uint killerShipId)
    {
        if (!killed || killedObject->is_player())
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }

        const uint killerClientId = HkGetClientIDByShip(killerShipId);
        if (!killerClientId)
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }

        uint victimReputationId;
        if (killedObject->cobj->objectClass == CObject::CSHIP_OBJECT)
        {
            victimReputationId = reinterpret_cast<CEqObj*>(killedObject->cobj)->repVibe;
        }
        else if (killedObject->cobj->objectClass == CObject::CSOLAR_OBJECT)
        {
            // Solars have their nickname ID directly mapped to their Reputation ID
            victimReputationId = killedObject->cobj->id;
        }
        else
        {
            returncode = DEFAULT_RETURNCODE;
            return;
        }

        uint victimGroupId;
        Reputation::Vibe::GetAffiliation(victimReputationId, victimGroupId, false);

        ChangeReputationOfGroup(killerClientId, victimGroupId);
        returncode = DEFAULT_RETURNCODE;
    }
}