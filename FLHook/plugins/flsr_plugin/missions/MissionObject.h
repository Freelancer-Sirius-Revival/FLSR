#pragma once

namespace Missions
{
	enum class MissionObjectType
	{
		Object,
		Client
	};

	struct MissionObject
	{
		MissionObjectType type = MissionObjectType::Client;
		unsigned int id = 0;
		MissionObject(const MissionObjectType type, unsigned int id) :
			type(type),
			id(id)
		{}
		bool operator == (const MissionObject& other) const
		{
			return type == other.type && id == other.id;
		}
	};
}