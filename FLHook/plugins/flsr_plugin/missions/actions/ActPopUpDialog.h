#pragma once
#include "Action.h"
#include "../Mission.h"

namespace Missions
{
	struct ActPopUpDialog : Action
	{
		uint label = 0;
		uint headingId = 0;
		uint messageId = 0;
		uint buttons = PopupDialogButton::CENTER_OK;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActPopUpDialog> ActPopUpDialogPtr;
}