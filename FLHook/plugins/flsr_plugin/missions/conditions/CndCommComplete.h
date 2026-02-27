#pragma once
#include "Condition.h"

namespace Missions
{
	class CndCommComplete : public Condition
	{
	private:
		const uint commName;

	public:
		CndCommComplete(const ConditionParent& parent, const uint commName);
		~CndCommComplete();
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