#include "ActStartDialog.h"
#include "ActEtherComm.h"
#include "ActSendComm.h"

namespace Missions
{
	static void PlayComm(Mission& mission, const MissionObject activator, const Dialog& dialog, const size_t lineIndex)
	{
		if (lineIndex + 1 < dialog.lines.size())
		{
			const ConditionPtr condition = ConditionPtr(new ActDialogCndCommComplete(mission, activator, dialog, lineIndex));
			condition->Register();
			mission.dynamicConditions.insert({ condition.get(), condition });
		}

		const auto& line = dialog.lines[lineIndex];
		const auto& senderEntry = dialog.etherSenders.find(line.senderEtherSenderOrObjName);
		if (senderEntry != dialog.etherSenders.end())
		{
			ActEtherComm comm;
			comm.id = line.id;
			comm.senderVoiceId = senderEntry->second.voiceId;
			comm.senderIdsName = senderEntry->second.idsName;
			comm.receiverObjNameOrLabel = line.receiverObjNameOrLabel;
			comm.lines = line.lines;
			comm.delay = line.delay;
			comm.costume = senderEntry->second.costume;
			comm.global = line.global;
			comm.Execute(mission, activator);
		}
		else
		{
			ActSendComm comm;
			comm.id = line.id;
			comm.senderObjName = line.senderEtherSenderOrObjName;
			comm.receiverObjNameOrLabel = line.receiverObjNameOrLabel;
			comm.lines = line.lines;
			comm.delay = line.delay;
			comm.global = line.global;
			comm.Execute(mission, activator);
		}
	}

	ActDialogCndCommComplete::ActDialogCndCommComplete(Mission& mission, const MissionObject& originalActivator, const Dialog& dialog, const int lineIndex) :
		CndCommComplete(ConditionParent(mission.id, 0), dialog.lines[lineIndex].id),
		mission(mission),
		originalActivator(originalActivator),
		dialog(dialog),
		lineIndex(lineIndex)
	{}

	void ActDialogCndCommComplete::ExecuteTrigger()
	{
		const size_t nextLineIndex = lineIndex + 1;
		if (nextLineIndex < dialog.lines.size())
			PlayComm(mission, originalActivator, dialog, nextLineIndex);
		mission.dynamicConditions.erase(this);
	}

	void ActStartDialog::Execute(Mission& mission, const MissionObject& activator) const
	{
		const auto& dialogEntry = mission.dialogs.find(dialogId);
		if (dialogEntry == mission.dialogs.end())
			return;
		const auto& dialog = dialogEntry->second;

		if (!dialog.lines.empty())
			PlayComm(mission, activator, dialog, 0);
	}
}