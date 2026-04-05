#include "CndCommComplete.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndCommComplete*> observedCndCommComplete;
	std::vector<CndCommComplete*> orderedCndCommComplete;

	CndCommComplete::CndCommComplete(const ConditionParent& parent, const uint commName, const uint activatorLabel) :
		Condition(parent),
		commName(commName),
		activatorLabel(activatorLabel)
	{}

	CndCommComplete::~CndCommComplete()
	{
		Unregister();
	}

	ConditionPtr CndCommComplete::Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const
	{
		return ConditionPtr(new CndCommComplete(newParent, commName, overrideObjNameOrLabel));
	}

	void CndCommComplete::Register()
	{
		if (observedCndCommComplete.insert(this).second)
			orderedCndCommComplete.push_back(this);
	}

	void CndCommComplete::Unregister()
	{
		observedCndCommComplete.erase(this);
		if (const auto it = std::find(orderedCndCommComplete.begin(), orderedCndCommComplete.end(), this); it != orderedCndCommComplete.end())
			orderedCndCommComplete.erase(it);
	}

	bool CndCommComplete::Matches(const uint capturedVoiceLineId, const uint capturedReceiverObjId)
	{
		auto& mission = missions.at(parent.missionId);
		const auto& commEntry = mission.ongoingComms.find(commName);
		if (commEntry != mission.ongoingComms.end() && commEntry->second.voiceLineId == capturedVoiceLineId && commEntry->second.receiverObjIds.contains(capturedReceiverObjId))
		{
			activator = commEntry->second.sender;
			if (activatorLabel != 0)
			{
				const auto& mission = missions.at(parent.missionId);
				const auto& objectsByLabel = mission.objectsByLabel.find(activatorLabel);
				if (objectsByLabel != mission.objectsByLabel.end() && !objectsByLabel->second.empty())
					activator = objectsByLabel->second.at(0);
			}
			mission.ongoingComms.erase(commEntry);
			return true;
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndCommComplete
		{
			void __stdcall CommComplete(unsigned int senderObjId, unsigned int receiverObjId, unsigned int voiceLineId, enum CommResult commResult)
			{
				const auto currentConditions(orderedCndCommComplete);
				for (const auto& condition : currentConditions)
				{
					if (observedCndCommComplete.contains(condition) && condition->Matches(voiceLineId, receiverObjId))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}