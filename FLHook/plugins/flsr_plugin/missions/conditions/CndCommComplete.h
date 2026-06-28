#pragma once
#include "Condition.h"

namespace Missions
{
	class CndCommComplete : public Condition
	{
	private:
		const uint activatorLabel;
		const uint commName;

	public:
		CndCommComplete(const ConditionParent& parent, const uint commName, const uint activatorLabel = 0);
		~CndCommComplete();
		ConditionPtr Copy(const ConditionParent& newParent, const uint overrideObjNameOrLabel) const;
		void Register();
		void Unregister();
		bool Matches(const uint capturedVoiceLineId, const uint capturedReceiverObjId);
	};

	namespace Hooks
	{
		namespace CndCommComplete
		{
			enum CommResult;
			void __stdcall CommComplete(unsigned int senderObjId, unsigned int receiverObjId, unsigned int voiceLineId, CommResult commResult);
		}
	}
}