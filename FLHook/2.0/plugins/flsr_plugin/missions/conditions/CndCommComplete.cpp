#include "CndCommComplete.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndCommComplete*> observedCndCommComplete;

	CndCommComplete::CndCommComplete(const ConditionParent& parent, const uint commName) :
		Condition(parent),
		commName(commName)
	{}

	CndCommComplete::~CndCommComplete()
	{
		Unregister();
	}

	void CndCommComplete::Register()
	{
		observedCndCommComplete.insert(this);
	}

	void CndCommComplete::Unregister()
	{
		observedCndCommComplete.erase(this);
	}

	bool CndCommComplete::Matches(const uint capturedVoiceLineId, const uint capturedReceiverObjId)
	{
		auto& mission = missions.at(parent.missionId);
		const auto& commEntry = mission.ongoingComms.find(commName);
		if (commEntry != mission.ongoingComms.end() && commEntry->second.voiceLineId == capturedVoiceLineId && commEntry->second.receiverObjIds.contains(capturedReceiverObjId))
		{
			activator = commEntry->second.sender;
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
				returncode = DEFAULT_RETURNCODE;

				const std::unordered_set<Missions::CndCommComplete*> currentConditions(observedCndCommComplete);
				for (const auto& condition : currentConditions)
				{
					if (observedCndCommComplete.contains(condition) && condition->Matches(voiceLineId, receiverObjId))
						condition->ExecuteTrigger();
				}
			}
		}
	}
}