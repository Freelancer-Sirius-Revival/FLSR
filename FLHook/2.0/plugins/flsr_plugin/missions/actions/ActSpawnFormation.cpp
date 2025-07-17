#include "ActSpawnFormation.h"
#include "MissionShipSpawning.h"
#include "../Formations.h"
#include "../MatrixMath.h"

namespace Missions
{
	void ActSpawnFormation::Execute(Mission& mission, const MissionObject& activator) const
	{
		const auto& formationEntry = mission.formations.find(msnFormationId);
		if (formationEntry == mission.formations.end())
			return;

		const auto& msnFormation = formationEntry->second;
		const auto& formation = Formations::GetFormation(msnFormation.formationId);
		const Vector& actualPosition = position.x != std::numeric_limits<float>::infinity() ? position : msnFormation.position;
		const Vector& actualRotation = rotation.x != std::numeric_limits<float>::infinity() ? rotation : msnFormation.rotation;
		const auto& translationMatrix = SetupTransform(actualPosition, { 0, 0, 0 });

		std::vector<uint> spawnedShipIds;
		for (int index = 0, length = min(formation.size(), msnFormation.msnShipIds.size()); index < length; index++)
		{
			// Move the ships to the right spots locally and rotate the entirety of this to what we want in the end.
			auto formationMatrix = SetupTransform(formation[index], actualRotation);
			// Move the locally transformed formation to the point in space where we want it in the end.
			formationMatrix = MultiplyMatrix(formationMatrix, translationMatrix);
			// Freelancer's ship rotation axis are inverted so we have to use a specific rotation matrix for those.
			const auto& shipRotationMatrix = SetupTransform({ 0, 0, 0 }, { -actualRotation.x, -actualRotation.y, -actualRotation.z });

			const Vector position = { formationMatrix.d[3][0], formationMatrix.d[3][1], formationMatrix.d[3][2] };
			Matrix orientation;
			orientation.data[0][0] = shipRotationMatrix.d[0][0];
			orientation.data[0][1] = shipRotationMatrix.d[0][1];
			orientation.data[0][2] = shipRotationMatrix.d[0][2];
			orientation.data[1][0] = shipRotationMatrix.d[1][0];
			orientation.data[1][1] = shipRotationMatrix.d[1][1];
			orientation.data[1][2] = shipRotationMatrix.d[1][2];
			orientation.data[2][0] = shipRotationMatrix.d[2][0];
			orientation.data[2][1] = shipRotationMatrix.d[2][1];
			orientation.data[2][2] = shipRotationMatrix.d[2][2];

			const uint objId = SpawnShip(msnFormation.msnShipIds[index], mission, &position, &orientation);
			if (objId)
				spawnedShipIds.push_back(objId);
		}

		int leaderIndex = -1;
		for (int index = 0, length = spawnedShipIds.size(); index < length; index++)
		{
			if (spawnedShipIds[index] == 0)
				continue;

			if (leaderIndex < 0)
			{
				leaderIndex = index;
				continue;
			}

			pub::AI::DirectiveFollowOp followOp;
			followOp.fireWeapons = false;
			followOp.followSpaceObj = spawnedShipIds[leaderIndex];
			followOp.maxDistance = 150.0f;
			//followOp.dunno2 = 400.0f;
			followOp.offset.x = formation[index].x - formation[leaderIndex].x;
			followOp.offset.y = formation[index].y - formation[leaderIndex].y;
			followOp.offset.z = formation[index].z - formation[leaderIndex].z;
			pub::AI::SubmitDirective(spawnedShipIds[index], &followOp);
		}

		if (leaderIndex >= 0)
		{			
			if (const auto& objectivesEntry = mission.objectives.find(objectivesId); objectivesEntry != mission.objectives.end())
			{
				const uint leaderId = spawnedShipIds[leaderIndex];
				mission.objectivesByObjectId.try_emplace(leaderId, mission.id, leaderId, objectivesEntry->second.objectives);
				mission.objectivesByObjectId.at(leaderId).Progress();
			}
		}
	}
}