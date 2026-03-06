#pragma once

namespace RandomMissions
{
	void ReadTradeCommoditiesData();
	void CacheDockableSolars();

	namespace Hooks
	{
		namespace TradeMissions
		{
			void __stdcall BaseEnter(unsigned int baseId, unsigned int clientId);
			void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
		}
	}
}
