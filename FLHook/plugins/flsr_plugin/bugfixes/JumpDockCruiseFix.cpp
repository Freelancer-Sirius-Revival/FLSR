#pragma once
#include "JumpDockCruiseFix.h"

namespace JumpDockCruiseFix
{
	int __cdecl Dock_Call_After(unsigned int const& shipId, unsigned int const& dockTargetId, int dockPortIndex, DOCK_HOST_RESPONSE response)
	{
		if (response != DOCK_HOST_RESPONSE::PROCEED_DOCK)
		{
			returncode = DEFAULT_RETURNCODE;
			return 0;
		}

		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(dockTargetId, inspect, system) || !(inspect->cobj->type == ObjectType::JumpGate || inspect->cobj->type == ObjectType::JumpHole))
		{
			returncode = DEFAULT_RETURNCODE;
			return 0;
		}

		if (GetShipInspect(shipId, inspect, system) && inspect->is_player())
		{
			const uint clientId = inspect->cobj->ownerPlayer;
			XActivateCruise data;
			data.iShip = shipId;
			data.bActivate = false;
			Server.ActivateCruise(clientId, data);
			GetClientInterface()->Send_FLPACKET_COMMON_ACTIVATECRUISE(clientId, data);
		}

		returncode = DEFAULT_RETURNCODE;
		return 0;
	}

	void __stdcall JumpInComplete_After(unsigned int systemId, unsigned int shipId)
	{
		IObjRW* inspect;
		StarSystem* system;
		if (GetShipInspect(shipId, inspect, system) && inspect->is_player())
		{
			const uint clientId = inspect->cobj->ownerPlayer;
			reinterpret_cast<CShip*>(inspect->cobj)->set_throttle(0.0f);

			SSPObjUpdateInfo serverData;
			serverData.iShip = shipId;
			serverData.vDir = HkMatrixToQuaternion(inspect->cobj->mRot);
			serverData.vPos = inspect->cobj->vPos;
			serverData.fTimestamp = 0.0f;
			serverData.throttle = 0.0f;
			serverData.cState = 0x13;
			Server.SPObjUpdate(serverData, clientId);

			SSPObjUpdateInfoClient clientData;
			clientData.iShip = shipId;
			clientData.vDir = HkMatrixToQuaternion(inspect->cobj->mRot);
			clientData.vPos = inspect->cobj->vPos;
			clientData.fTimestamp = 0.0f;
			clientData.throttle = 0.0f;
			clientData.state = 0x13;
			GetClientInterface()->Send_FLPACKET_COMMON_UPDATEOBJECT(clientId, clientData);
		}
	}
}