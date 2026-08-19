#pragma once
#include "Condition.h"

namespace Missions
{
	class CndPopUpDialog : public Condition
	{
	private:
		const uint label;
		const uint popupName;
		const uint targetButton;

	public:
		CndPopUpDialog(const ConditionParent& parent, const uint label, const uint popupName, const uint targetButton);
		~CndPopUpDialog();
		ConditionPtr Copy(const ConditionParent& newParent, const uint overrideLabel) const;
		void Register();
		void Unregister();
		bool Matches(const uint clientId, const uint buttonClicked);
	};

	namespace Hooks
	{
		namespace CndPopUpDialog
		{
			void __stdcall PopUpDialog_AFTER(unsigned int clientId, unsigned int buttonClicked);
		}
	}
}