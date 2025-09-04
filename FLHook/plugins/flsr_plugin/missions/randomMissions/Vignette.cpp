#include "Vignette.h"
#include <optional>

namespace RandomMissions
{
	std::unordered_map<uint, std::vector<Vignette>> vignettesBySystemId;

	void ReadVignetteData() {
		INI_Reader ini;
		if (!ini.open("..\\DATA\\UNIVERSE\\universe.ini", false)) return;

		std::unordered_map<uint, std::string> pathBySystemId;

		while (ini.read_header())
		{
			if (ini.is_header("system"))
			{
				uint id = 0;
				std::string path;
				while (ini.read_value())
				{
					if (ini.is_value("nickname"))
					{
						id = CreateID(ini.get_value_string(0));
					}
					else if (ini.is_value("file"))
					{
						path = ini.get_value_string(0);
					}
				}
				if (id && !path.empty())
				{
					pathBySystemId.insert({ id, path });
				}
			}
		}
		ini.close();

		for (const auto& [systemId, path] : pathBySystemId)
		{
			if (!ini.open(("..\\DATA\\UNIVERSE\\" + path).c_str(), false))
			{
				return;
			}

			std::vector<Vignette> vignettes;
			std::vector<Zone> zones;

			while (ini.read_header())
			{
				if (ini.is_header("zone"))
				{
					Zone zone;
					zone.systemId = systemId;
					std::optional<VignetteType> vignette_type = std::nullopt;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
						{
							zone.id = CreateID(ini.get_value_string(0));
							zone.nickname = ini.get_value_string(0);
						}
						else if (ini.is_value("pos"))
						{
							zone.position = ini.get_vector();
						}
						else if (ini.is_value("size"))
						{
							zone.size = ini.get_vector();
						}
						else if (ini.is_value("vignette_type"))
						{
							std::string typeStr = ToLower(ini.get_value_string(0));
							if (typeStr == "open")
							{
								vignette_type = VignetteType::Open;
							}
							else if (typeStr == "field")
							{
								vignette_type = VignetteType::Field;
							}
							else if (typeStr == "exclusion")
							{
								vignette_type = VignetteType::Exclusion;
							}
						}
						else if (ini.is_value("shape"))
						{
							std::string typeStr = ToLower(ini.get_value_string(0));
							if (typeStr == "sphere")
							{
								zone.shape = Shape::Sphere;
							}
							else if (typeStr == "box")
							{
								zone.shape = Shape::Box;
							}
							else if (typeStr == "ellipsoid")
							{
								zone.shape = Shape::Ellipsoid;
							}
							else if (typeStr == "cylinder")
							{
								zone.shape = Shape::Cylinder;
							}
							else if (typeStr == "ring")
							{
								zone.shape = Shape::Ring;
							}
						}
						else if (ini.is_value("rotate"))
						{
							zone.rotation = ini.get_vector();
						}
						else if (ini.is_value("faction"))
						{
							zone.factionIds.insert(CreateID(ini.get_value_string(0)));
						}
					}
					if (zone.size.z == 0)
					{
						zone.size.z = zone.size.x;
					}
					if (zone.size.y == 0)
					{
						zone.size.y = zone.size.x;
					}
					if (zone.id && vignette_type.has_value())
					{
						Vignette vignette
						{
							zone, // supertype values
							zone.size.x, // diameter
							vignette_type.value() // type
						};
						vignettes.push_back(vignette);
					}
					else if (!zone.factionIds.empty())
					{
						zones.push_back(zone);
					}
				}
			}
			ini.close();

			for (Vignette& vignette : vignettes)
			{
				for (const Zone& zone : zones)
				{
					if (vignette.Intersects(zone))
					{
						vignette.factionIds.insert(zone.factionIds.begin(), zone.factionIds.end());
					}
				}
				if (!vignette.factionIds.empty())
				{
					vignettesBySystemId[systemId].push_back(vignette);
				}
			}
		}
	}

	static Vector VecAdd(const Vector& a, const Vector& b) {
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}
	static Vector VecSub(const Vector& a, const Vector& b) {
		return { a.x - b.x, a.y - b.y, a.z - b.z };
	}
	static Vector VecNeg(const Vector& a) {
		return { -a.x, -a.y, -a.z };
	}
	static Vector VecMul(const Vector& a, float b) {
		return { a.x * b, a.y * b, a.z * b };
	}
	static Vector VecMul(const Vector& a, const Vector& b) {
		return { a.x * b.x, a.y * b.y, a.z * b.z };
	}
	static Vector VecDiv(const Vector& a, float b) {
		return { a.x / b, a.y / b, a.z / b };
	}
	static Vector VecDiv(const Vector& a, const Vector& b) {
		return { a.x / b.x, a.y / b.y, a.z / b.z };
	}
	static float VecDistanceToOriginSquared(const Vector& a) {
		return a.x * a.x + a.y * a.y + a.z * a.z;
	}
	static float VecDistanceSquared(const Vector& a, const Vector& b) {
		return VecDistanceToOriginSquared(VecSub(a, b));
	}
	static Vector VecClamp(const Vector& a, const Vector& min, const Vector& max) {
		return {
			std::fmax(min.x, std::fmin(a.x, max.x)),
			std::fmax(min.y, std::fmin(a.y, max.y)),
			std::fmax(min.z, std::fmin(a.z, max.z))
		};
	}

	static const float PI = 3.14159265358979323846f;
	static float DegToRad(const float degrees) {
		return degrees * (PI / 180.0f);
	}
	static Vector VecRotX(const Vector& point, const float angleRad) {
		const float sinA = std::sin(angleRad);
		const float cosA = std::cos(angleRad);
		return {
			point.x,
			point.y * cosA - point.z * sinA,
			point.z * cosA + point.y * sinA
		};
	}
	static Vector VecRotY(const Vector& point, const float angleRad) {
		const float sinA = std::sin(angleRad);
		const float cosA = std::cos(angleRad);
		return {
			point.x * cosA + point.z * sinA,
			point.y,
			point.z * cosA - point.x * sinA
		};
	}
	static Vector VecRotZ(const Vector& point, const float angleRad) {
		const float sinA = std::sin(angleRad);
		const float cosA = std::cos(angleRad);
		return {
			point.x * cosA - point.y * sinA,
			point.y * cosA + point.x * sinA,
			point.z
		};
	}
	static Vector VecRelativeTo(const Vector& point, const Vector& rotationCenter, const Vector& eulerDegrees) {
		// Move point relative to center
		Vector local = VecSub(point, rotationCenter);

		// Apply inverse rotations in Z-Y-X order
		local = VecRotZ(local, DegToRad(-eulerDegrees.z));
		local = VecRotY(local, DegToRad(-eulerDegrees.y));
		local = VecRotX(local, DegToRad(-eulerDegrees.x));

		// Return transformed point in local space (still relative to box center)
		return local;
	}

	bool Vignette::Intersects(const Zone& zone) const
	{
		switch (zone.shape)
		{
			case Shape::Sphere: return IntersectsSphere(zone.position, zone.size.x);
			case Shape::Box: return IntersectsBox(zone.position, zone.size, zone.rotation);
			case Shape::Ellipsoid: return IntersectsEllipsoid(zone.position, zone.size, zone.rotation);
			case Shape::Cylinder: return IntersectsCylinder(zone.position, zone.size.x, zone.size.y, zone.rotation);
			case Shape::Ring: return IntersectsRing(zone.position, zone.size.x, zone.size.y, zone.size.z, zone.rotation);
			default: return false;
		}
	}

	bool Vignette::IntersectsSphere(const Vector& zone_position, const float zone_diameter) const
	{
		// For two spheres to touch, their centers cannot be further apart than the radii of both spheres added together.
		float radius_sum = (diameter + zone_diameter) / 2;
		// We compare squared distances because nobody likes square roots.
		return VecDistanceSquared(position, zone_position) <= radius_sum * radius_sum;
	}

	bool Vignette::IntersectsBox(const Vector& zone_position, const Vector& zone_size, const Vector& zone_rotation) const
	{
		// First, we calculate the vignette center relative to the local space of the box.
		// This means that every calculation from now on can assume that the box is axis-aligned and its position is at the origin { 0, 0, 0 }.
		Vector position_relative = VecRelativeTo(position, zone_position, zone_rotation);
		
		// Next, we find the point inside the box's bounds that is closest to the vignette center.
		// This is done by clamping each coordinate of the vignette center to the box's min and max coordinates.
		Vector zone_half_size = VecDiv(zone_size, 2);
		Vector closest_point_in_zone = VecClamp(position_relative, VecNeg(zone_half_size), zone_half_size);
		
		// Finally, we check if the distance between this closest point and the vignette center is inside the vignette.
		// Once again, we compare squared distances to avoid a square root.
		float radius = diameter / 2;
		return VecDistanceSquared(position_relative, closest_point_in_zone) <= radius * radius;
	}

	bool Vignette::IntersectsEllipsoid(const Vector& zone_position, const Vector& zone_size, const Vector& zone_rotation) const
	{
		// As with boxes, we put the vignette center into the local space of the ellipsoid, so the ellipsoid is axis-aligned and at the origin.
		Vector position_relative = VecRelativeTo(position, zone_position, zone_rotation);

		// Now, we calculate the direction from the zone center to the vignette center as a unit vector.
		float distance = std::sqrt(VecDistanceToOriginSquared(position_relative));
		Vector dir = VecDiv(position_relative, distance);

		// We use that direction to calculate what the "radius" of the zone is in this direction ("ellipsoidal radius projection").
		float effective_radius = std::sqrt(VecDistanceToOriginSquared(VecMul(zone_size, dir)));

		// Now that we have the effective radius, we can check if the vignette's radius and the zone's effective radius overlap,
		// which is done the same way as with two spheres.
		return distance <= diameter / 2 + effective_radius;
	}

	bool Vignette::IntersectsCylinder(const Vector& zone_position, const float zone_diameter, const float zone_height, const Vector& zone_rotation) const
	{
		// This one needs some explaining.
		// So, turns out that calculating overlap between a sphere and a cylinder directly is not as trivial as I thought it would be.
		// However, what's easier is calculating overlap between a sphere and a capsule (basically a cylinder, but the ends are hemispheres and not flat).
		// At some point, I also realized that a cylinder is just the intersection between a capsule and a box (basically "clipping off" the hemispherical ends).
		// So basically, for a cylinder with diameter D and height H, we can simply check if the vignette intersects:
		// - a capsule with diameter D and height H+D (including the hemispherical ends),
		// - AND a box with size { D, H, D } centered at the same position,
		// and if both are true, then the vignette intersects the cylinder.
		// I'm actually quite proud of coming up with this on my own :3 Apparently I'm not COMPLETELY useless at geometry after all.

		// Once again, transform vignette center into the zone's local space.
		Vector position_relative = VecRelativeTo(position, zone_position, zone_rotation);

		// We do the box check first, you'll see why later. (If you need an explanation for this part, see the `IntersectsBox` function)
		Vector box_half_size = VecDiv({ zone_diameter, zone_height, zone_diameter }, 2);
		Vector closest_point_in_box = VecClamp(position_relative, VecNeg(box_half_size), box_half_size);
		float radius = diameter / 2;
		if (VecDistanceSquared(position_relative, closest_point_in_box) > radius * radius)
		{
			// If the box check fails, we can return early.
			return false;
		}

		// Now for the capsule check. It's actually quite simple.
		// First, we find the point on the capsule's central axis (the part of the Y-axis that's inside the cylindrical part) closest to the vignette center.
		// This is done by clamping the vignette center's Y-coordinate to the cylinder's (NOT the capsule's) height.
		// Since we actually already did that same calculation in the box check, we can simply reuse the Y-coordinate from that.
		Vector closest_point_on_axis = { 0, closest_point_in_box.y, 0 };

		// Now we calculate the distance between that point and the vignette center.
		float distance_to_axis_squared = VecDistanceSquared(position_relative, closest_point_on_axis);

		// Since we now have the shortest distance, we can treat the "closest point" as the center of a sphere, which makes the final condition trivial.
		float radius_sum = radius + zone_diameter / 2;
		// Once again, we compare squared distances to avoid a square root.
		return distance_to_axis_squared <= radius_sum * radius_sum;
	}

	bool Vignette::IntersectsRing(const Vector& zone_position, const float zone_diameter, const float zone_diameter_inner, const float zone_height, const Vector& zone_rotation) const
	{
		// This could be done more efficiently by reusing some calculations between the two cylinder checks,
		// but since this will likely never be used at all, I'll leave it as is for now.
		return IntersectsCylinder(zone_position, zone_diameter, zone_height, zone_rotation) && !IntersectsCylinder(zone_position, zone_diameter_inner, zone_height, zone_rotation);
	}
}