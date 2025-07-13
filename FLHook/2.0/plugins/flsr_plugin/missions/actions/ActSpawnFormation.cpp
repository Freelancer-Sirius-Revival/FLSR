#include "ActSpawnFormation.h"
#include "../Formations.h"
#include "ActSpawnShip.h"

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
		const auto& translationMatrix = ZoneUtilities::SetupTransform(actualPosition, { 0, 0, 0 });

		for (int index = 0, length = min(formation.size(), msnFormation.msnShipIds.size()); index < length; index++)
		{
			// Move the ships to the right spots locally and rotate the entirety of this to what we want in the end.
			auto formationMatrix = ZoneUtilities::SetupTransform(formation[index], actualRotation);
			// Move the locally transformed formation to the point in space where we want it in the end.
			formationMatrix = ZoneUtilities::MultiplyMatrix(formationMatrix, translationMatrix);
			// Freelancer's ship rotation axis are inverted so we have to use a specific rotation matrix for those.
			const auto& shipRotationMatrix = ZoneUtilities::SetupTransform({ 0, 0, 0 }, { -actualRotation.x, -actualRotation.y, -actualRotation.z });

			ActSpawnShip action;
			action.msnNpcId = msnFormation.msnShipIds[index];
			action.position.x = formationMatrix.d[3][0];
			action.position.y = formationMatrix.d[3][1];
			action.position.z = formationMatrix.d[3][2];
			action.orientation.data[0][0] = shipRotationMatrix.d[0][0];
			action.orientation.data[0][1] = shipRotationMatrix.d[0][1];
			action.orientation.data[0][2] = shipRotationMatrix.d[0][2];
			action.orientation.data[1][0] = shipRotationMatrix.d[1][0];
			action.orientation.data[1][1] = shipRotationMatrix.d[1][1];
			action.orientation.data[1][2] = shipRotationMatrix.d[1][2];
			action.orientation.data[2][0] = shipRotationMatrix.d[2][0];
			action.orientation.data[2][1] = shipRotationMatrix.d[2][1];
			action.orientation.data[2][2] = shipRotationMatrix.d[2][2];
			action.Execute(mission, activator);
		}
	}
}