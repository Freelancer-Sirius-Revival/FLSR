#pragma once
#include <FLHook.h>

namespace RandomMissions
{
	enum class VignetteType { Open, Field, Exclusion };

	struct ZoneBase
	{
		uint id;
		std::string nickname;
		uint systemId;
		Vector position = { 0, 0, 0 };
		std::unordered_set<uint> factionIds;
	};

	enum class Shape {
		Sphere, Box, Ellipsoid, Cylinder, Ring
	};
	struct Zone : public ZoneBase
	{
		Vector rotation = { 0, 0, 0 };
		Shape shape = Shape::Sphere;
		Vector size = { 0, 0, 0 };
	};

	struct Vignette : public ZoneBase
	{
		float diameter;
		VignetteType type;

		bool Intersects(const Zone& zone) const;
		bool IntersectsSphere(const Vector& zone_position, float zone_diameter) const;
		bool IntersectsBox(const Vector& zone_position, const Vector& zone_size, const Vector& zone_rotation) const;
		bool IntersectsEllipsoid(const Vector& zone_position, const Vector& zone_size, const Vector& zone_rotation) const;
		bool IntersectsCylinder(const Vector& zone_position, float zone_diameter, float zone_height, const Vector& zone_rotation) const;
		bool IntersectsRing(const Vector& zone_position, float zone_diameter, float zone_diameter_inner, float zone_height, const Vector& zone_rotation) const;
	};

	extern std::unordered_map<uint, std::vector<Vignette>> vignettesBySystemId;

	void ReadVignetteData();
}