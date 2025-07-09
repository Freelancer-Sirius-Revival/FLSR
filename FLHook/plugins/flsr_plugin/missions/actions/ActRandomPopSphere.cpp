#include "ActRandomPopSphere.h"
#include "../../Plugin.h"

namespace Missions
{
	struct Sphere
	{
		Vector position;
		float distance;
		uint missionId;
	};
	std::unordered_map<uint, std::vector<Sphere>> spheresBySystemId;

	void ActRandomPopSphere::Execute(Mission& mission, const MissionObject& activator) const
	{
		bool sphereFound = false;
		if (const auto& systemEntry = spheresBySystemId.find(systemId); systemEntry != spheresBySystemId.end())
		{
			for (auto it = systemEntry->second.begin(); it != systemEntry->second.end();)
			{
				const auto& sphere = *it;
				sphereFound = sphere.missionId == mission.id &&
								sphere.distance == distance &&
								sphere.position.x == position.x &&
								sphere.position.y == position.y &&
								sphere.position.z == position.z;
				if (!sphereFound)
				{
					it++;
				}
				else
				{
					if (spawningAllowed)
						it = systemEntry->second.erase(it);
					else
						break;
				}
			}
		}

		if (!spawningAllowed && !sphereFound)
		{
			Sphere sphere;
			sphere.distance = distance;
			sphere.position = position;
			sphere.missionId = mission.id;
			spheresBySystemId[systemId].push_back(sphere);
		}
	}

	void ClearRandomPopSpheres(const uint missionId)
	{
		for (auto& systemEntry : spheresBySystemId)
		{
			for (auto it = systemEntry.second.begin(); it != systemEntry.second.end();)
			{
				if (it->missionId == missionId)
					it = systemEntry.second.erase(it);
				else
					it++;
			}
		}

		for (auto it = spheresBySystemId.begin(); it != spheresBySystemId.end();)
		{
			if (it->second.empty())
				it = spheresBySystemId.erase(it);
			else
				it++;
		}
	}

	void __stdcall CShip_init(CShip* ship)
	{
		returncode = DEFAULT_RETURNCODE;

		if (ship->ownerPlayer != 0)
			return;

		for (const auto& mission : missions)
		{
			if (mission.second.objectIds.contains(ship->id))
				return;
		}

		//if (ship->ownerPlayer != 0 || pub::SpaceObj::ExistsAndAlive(ship->id) != 0)
		//	return;

		if (const auto& systemEntry = spheresBySystemId.find(ship->system); systemEntry != spheresBySystemId.end())
		{
			for (const auto& sphere : systemEntry->second)
			{
				if (HkDistance3D(sphere.position, ship->vPos) < sphere.distance)
				{
					pub::SpaceObj::Destroy(ship->id, DestroyType::VANISH);
					return;
				}
			}
		}
	}
}