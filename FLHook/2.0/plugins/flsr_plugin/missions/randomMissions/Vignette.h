#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	enum class VignetteType : uint8_t
	{
		Open = 1, Field = 2, Exclusion = 3
	};
	enum class VignetteTypeOpt : uint8_t
	{
		None = 0,
		Open = VignetteType::Open,
		Field = VignetteType::Field,
		Exclusion = VignetteType::Exclusion,
	};

	struct ZoneBase
	{
		uint id;
		uint systemId;
		Vector position;
		std::unordered_set<uint> factionIds;
	};

	enum class Shape {
		Sphere, Box, Ellipsoid, Cylinder, Ring
	};
	struct Zone : public ZoneBase
	{
		Vector rotation;
		Shape shape;
		Vector size;
		VignetteTypeOpt type = VignetteTypeOpt::None;
	};

	struct Vignette : public ZoneBase
	{
		float diameter;
		VignetteType type;
		bool Intersects(const Zone& zone) const;
		bool IntersectsSphere(Vector zone_position, float zone_diameter) const;
		bool IntersectsBox(Vector zone_position, Vector zone_size, Vector zone_rotation) const;
		bool IntersectsEllipsoid(Vector zone_position, Vector zone_size, Vector zone_rotation) const;
		bool IntersectsCylinder(Vector zone_position, float zone_diameter, float zone_height, Vector zone_rotation) const;
		bool IntersectsRing(Vector zone_position, float zone_diameter, float zone_diameter_inner, float zone_height, Vector zone_rotation) const;
	};

	extern std::unordered_map<uint, std::vector<Vignette>> vignettesBySystemId;

	void ReadVignetteData();
}