#pragma once

namespace Empathies
{
    enum class ReputationChangeReason
    {
        ObjectDestruction,
        MissionSuccess,
        MissionFailure,
        MissionAbortion
    };

    void ChangeReputationsByValue(const unsigned int clientId, const unsigned int groupId, const float change);
	void ChangeReputationsByReason(const unsigned int clientId, const unsigned int groupId, const ReputationChangeReason reason);
	void Initialize();
}