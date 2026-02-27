#pragma once
#include "Condition.h"

namespace Missions
{
	class CndHasCargo : public Condition
	{
	private:
		const uint label;
		const std::unordered_map<uint, uint> countPerCargo;

		bool HasCargo(const uint clientId);

	public:
		CndHasCargo(const ConditionParent& parent, const uint label, const std::unordered_map<uint, uint>& countPerCargo);
		~CndHasCargo();
		void Register();
		void Unregister();
		bool Matches(const uint clientId);
	};

	namespace Hooks
	{
		namespace CndHasCargo
		{
			void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
			void __stdcall ReqAddItem_AFTER(unsigned int& goodArchetypeId, char* hardpoint, int& count, float& status, bool& mounted, uint clientId);
			void __stdcall ReqEquipment_AFTER(const EquipDescList& equipDescriptorList, unsigned int clientId);
		}
	}
}