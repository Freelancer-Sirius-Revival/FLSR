#include "CndPopUpDialog.h"
#include "../Mission.h"
#include "../../Plugin.h"

namespace Missions
{
	std::unordered_set<CndPopUpDialog*> observedCndPopUpDialog;
	std::vector<CndPopUpDialog*> orderedCndPopUpDialog;

	CndPopUpDialog::CndPopUpDialog(const ConditionParent& parent, const uint label, const uint popupName, const uint targetButton) :
		Condition(parent),
		label(label),
		popupName(popupName),
		targetButton(targetButton)
	{}

	CndPopUpDialog::~CndPopUpDialog()
	{
		Unregister();
	}

	ConditionPtr CndPopUpDialog::Copy(const ConditionParent& newParent, const uint overrideLabel) const
	{
		return ConditionPtr(new CndPopUpDialog(newParent, overrideLabel != 0 ? overrideLabel : label, popupName, targetButton));
	}

	void CndPopUpDialog::Register()
	{
		if (observedCndPopUpDialog.insert(this).second)
			orderedCndPopUpDialog.push_back(this);
	}

	void CndPopUpDialog::Unregister()
	{
		observedCndPopUpDialog.erase(this);
		if (const auto it = std::find(orderedCndPopUpDialog.begin(), orderedCndPopUpDialog.end(), this); it != orderedCndPopUpDialog.end())
			orderedCndPopUpDialog.erase(it);
	}

	bool CndPopUpDialog::Matches(const uint clientId, const uint buttonClicked)
	{
		if (targetButton != buttonClicked)
			return false;

		const auto& mission = missions.at(parent.missionId);

		if (const auto& entry = mission.lastOpenedPopUpNameByClientId.find(clientId); entry == mission.lastOpenedPopUpNameByClientId.end() || entry->second != popupName)
			return false;

		if (label == Stranger)
		{
			if (!mission.clientIds.contains(clientId))
			{
				activator.type = MissionObjectType::Client;
				activator.id = clientId;
				return true;
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(label); objectsByLabel != mission.objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client && object.id == clientId)
				{
					activator = object;
					return true;
				}
			}
		}
		return false;
	}

	namespace Hooks
	{
		namespace CndPopUpDialog
		{
			void __stdcall PopUpDialog_AFTER(unsigned int clientId, unsigned int buttonClicked)
			{
				const auto currentConditions(orderedCndPopUpDialog);
				for (const auto& condition : currentConditions)
				{
					if (observedCndPopUpDialog.contains(condition) && condition->Matches(clientId, buttonClicked))
						condition->ExecuteTrigger();
				}
				returncode = DEFAULT_RETURNCODE;
			}
		}
	}
}