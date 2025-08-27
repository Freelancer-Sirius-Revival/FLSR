#include "ActSpawnFormation.h"
#include "MissionShipSpawning.h"
#include "../ShipSpawning.h"
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

		std::vector<uint> msnShipIdsToSpawn;
		for (const auto& msnShipId : msnFormation.msnShipIds)
		{
			if (!mission.objectIdsByName.contains(msnShipId))
				msnShipIdsToSpawn.push_back(msnShipId);
		}

		std::vector<uint> spawnedShipIds;
		for (int index = 0, length = min(formation.size(), msnShipIdsToSpawn.size()); index < length; index++)
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

			const uint objId = SpawnShip(msnShipIdsToSpawn[index], mission, &position, &orientation);
			if (objId)
				spawnedShipIds.push_back(objId);
		}

		for (int index = 0, length = spawnedShipIds.size(); index < length; index++)
		{
			pub::AI::SetPersonalityParams personality;
			pub::AI::get_personality(spawnedShipIds[index], personality.personality);
			const uint graphId = pub::AI::get_state_graph_id(spawnedShipIds[index]); // This only works because all graphs will be LEADER type initially.
			personality.state_graph = pub::StateGraph::get_state_graph(graphId, index == 0 ? pub::StateGraph::TYPE_LEADER : pub::StateGraph::TYPE_ESCORT);
			personality.state_id = true;
			personality.contentCallback = 0;
			personality.directiveCallback = 0;
			pub::AI::SubmitState(spawnedShipIds[index], &personality);

			pub::AI::update_formation_state(spawnedShipIds[index], spawnedShipIds[0], formation[index]);
			ShipSpawning::AssignToWing(spawnedShipIds[index], spawnedShipIds[0]);
		}

		if (!spawnedShipIds.empty())
		{			
			if (const auto& objectivesEntry = mission.objectives.find(objectivesId); objectivesEntry != mission.objectives.end())
				objectivesEntry->second.Progress(ObjectiveState(spawnedShipIds[0], 0, false));
		}
	}
}