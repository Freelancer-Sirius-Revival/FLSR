#include "Main.h"
#include "AutoTurret.h"

namespace AutoTurret
{
	std::unordered_map<uint, std::unordered_set<uint>> projIdsByTargetId;

	void __fastcall CGuidedInit(CGuided* cguided, CGuided::CreateParms& param)
	{
		returncode = DEFAULT_RETURNCODE;
		if (param.target != nullptr)
			projIdsByTargetId[param.target->cobj->id].insert(param.id);
	}

	static void SetTarget(CEGun* gun, CSimple* targetObj)
	{
		CSimple* currentTarget;
		ushort subId;
		gun->GetTarget(currentTarget, subId);
		if ((currentTarget != nullptr && currentTarget->id == targetObj->id) || !gun->SetTarget(targetObj, 0))
			return;
		const auto& gunOwner = gun->owner;
		XSetTarget targetData;
		targetData.iShip = gunOwner->id;
		targetData.iSlot = gun->iSubObjId;
		targetData.iSpaceID = targetObj->id;
		targetData.iSubObjID = 0;
		const auto clientId = gunOwner->ownerPlayer;
		if (clientId > 0)
		{
			GetClientInterface()->Send_FLPACKET_COMMON_SETTARGET(clientId, targetData);
			Server.SetTarget(clientId, targetData);
		}
		else
		{
			struct PlayerData* playerData = 0;
			while (playerData = Players.traverse_active(playerData))
			{
				if (playerData->iSystemID == gunOwner->system)
					GetClientInterface()->Send_FLPACKET_COMMON_SETTARGET(playerData->iOnlineID, targetData);
			}
		}
	}

	static void FireGun(CEGun* gun, const Vector& targetPoint)
	{
		if (gun->Fire(targetPoint) != FireResult::Success)
			return;
		XFireWeaponInfo fireData;
		const auto& gunOwner = gun->owner;
		fireData.object = gunOwner->id;
		fireData.hpIds.push_back(gun->iSubObjId);
		fireData.target = targetPoint;
		const uint clientId = gunOwner->ownerPlayer;
		if (clientId > 0)
		{
			GetClientInterface()->Send_FLPACKET_COMMON_FIREWEAPON(clientId, fireData);
			Server.FireWeapon(clientId, fireData);
		}
		else
		{
			PlayerData* playerData = 0;
			while (playerData = Players.traverse_active(playerData))
			{
				if (playerData->iSystemID == gunOwner->system)
					GetClientInterface()->Send_FLPACKET_COMMON_FIREWEAPON(playerData->iOnlineID, fireData);
			}
		}
	}

	float elapsedTimeInSec = 0.0f;
	void __stdcall Elapse_Time_AFTER(float seconds)
	{
		returncode = DEFAULT_RETURNCODE;

		elapsedTimeInSec += seconds;
		if (elapsedTimeInSec < 0.05f)
			return;
		elapsedTimeInSec = 0.0f;

		for (auto it = projIdsByTargetId.begin(); it != projIdsByTargetId.end();)
		{
			uint defenderId = it->first;
			if (pub::SpaceObj::ExistsAndAlive(defenderId) != 0)
			{
				it = projIdsByTargetId.erase(it);
				continue;
			}

			for (auto projIdIt = it->second.begin(); projIdIt != it->second.end();)
			{
				if (pub::SpaceObj::ExistsAndAlive(*projIdIt) != 0)
					projIdIt = it->second.erase(projIdIt);
				else
					projIdIt++;
			}
			if (it->second.empty())
			{
				it = projIdsByTargetId.erase(it);
				continue;
			}

			StarSystem* system;
			IObjRW* defenderObjRw;
			if (!GetShipInspect(defenderId, defenderObjRw, system))
				continue;

			const auto& defenderObj = dynamic_cast<CEqObj*>(defenderObjRw->cobj);
			EquipDescVector equipList;
			defenderObj->get_equip_desc_list(equipList);
			std::unordered_set<CEGun*> guns;
			for (const auto& equip : equipList.equip)
			{
				const auto& cequip = defenderObj->equip_manager.FindByID(equip.sID);
				const auto& archetype = cequip->archetype;
				if (cequip->CEquipType == EquipmentClass::Gun && cequip->IsActive() && dynamic_cast<Archetype::Gun*>(archetype)->bAutoTurret)
					guns.insert(dynamic_cast<CEGun*>(cequip));
			}

			std::multimap<float, CSimple*> projectilesSortedByDistance;
			for (auto projectileId : it->second)
			{
				IObjRW* projectileObjRw;
				if (!GetShipInspect(projectileId, projectileObjRw, system) || defenderObj->system != projectileObjRw->cobj->system)
					continue;

				const auto& projectileObj = projectileObjRw->cobj;
				const auto distance = HkDistance3D(defenderObj->vPos, projectileObj->vPos);
				projectilesSortedByDistance.insert({ distance, projectileObj });
			}

			std::unordered_map<CEGun*, Vector> gunTargetPoints;
			for (const auto& entry : projectilesSortedByDistance)
			{
				for (auto gunIt = guns.begin(); gunIt != guns.end();)
				{
					const auto& gun = *gunIt;
					Vector targetPoint;
					gun->ComputeTgtLeadPosition(targetPoint);
					if (gun->CanPointAt(targetPoint, 0.0f))
					{
						SetTarget(gun, entry.second);
						gunTargetPoints[gun] = targetPoint;
						gunIt = guns.erase(gunIt);
					}
					else
						gunIt++;
				}
			}

			for (const auto& entry : gunTargetPoints)
			{
				if (HkDistance3D(entry.first->GetTurretOrigin(), entry.second) < entry.first->GetMunitionRange() * 1.1f)
					FireGun(entry.first, entry.second);
			}

			it++;
		}
	}
}