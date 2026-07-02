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
		MissionObjectType type;
		unsigned int id;
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