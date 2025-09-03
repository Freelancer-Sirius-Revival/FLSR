#pragma once
#include "Vignette.h"

namespace RandomMissions
{
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

			while (ini.read_header())
			{
				if (ini.is_header("zone"))
				{
					Zone zone{};
					zone.systemId = systemId;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
						{
							zone.id = CreateID(ini.get_value_string(0));
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
								zone.type = VignetteTypeOpt::Open;
							}
							else if (typeStr == "field")
							{
								zone.type = VignetteTypeOpt::Field;
							}
							else if (typeStr == "exclusion")
							{
								zone.type = VignetteTypeOpt::Exclusion;
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
					if (zone.id && zone.type != VignetteTypeOpt::None)
					{
						Vignette vignette
						{
							static_cast<ZoneBase>(zone), // supertype values
							zone.size.x, // diameter
							static_cast<VignetteType>(zone.type) // type
						};
						vignettes.push_back(vignette);
					}
				}
			}
			ini.close();
		}
	}

	Vector VecAdd(Vector a, Vector b) {
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	}
	Vector VecSub(Vector a, Vector b) {
		return { a.x - b.x, a.y - b.y, a.z - b.z };
	}
	Vector VecNeg(Vector a) {
		return { -a.x, -a.y, -a.z };
	}
	Vector VecMul(Vector a, float b) {
		return { a.x * b, a.y * b, a.z * b };
	}
	Vector VecMul(Vector a, Vector b) {
		return { a.x * b.x, a.y * b.y, a.z * b.z };
	}
	Vector VecDiv(Vector a, float b) {
		return { a.x / b, a.y / b, a.z / b };
	}
	Vector VecDiv(Vector a, Vector b) {
		return { a.x / b.x, a.y / b.y, a.z / b.z };
	}
	float VecDistanceToOriginSquared(Vector a) {
		return a.x * a.x + a.y * a.y + a.z * a.z;
	}
	float VecDistanceSquared(Vector a, Vector b) {
		return VecDistanceToOriginSquared(VecSub(a, b));
	}
	Vector VecClamp(Vector a, Vector min, Vector max) {
		return {
			std::fmax(min.x, std::fmin(a.x, max.x)),
			std::fmax(min.y, std::fmin(a.y, max.y)),
			std::fmax(min.z, std::fmin(a.z, max.z))
		};
	}

	const float PI = 3.14159265358979323846f;
	float DegToRad(float degrees) {
		return degrees * (PI / 180.0f);
	}
	Vector VecRotX(Vector point, float angleRad) {
		float sinA = std::sin(angleRad);
		float cosA = std::cos(angleRad);
		return {
			point.x,
			point.y * cosA - point.z * sinA,
			point.z * cosA + point.y * sinA
		};
	}
	Vector VecRotY(Vector point, float angleRad) {
		float sinA = std::sin(angleRad);
		float cosA = std::cos(angleRad);
		return {
			point.x * cosA + point.z * sinA,
			point.y,
			point.z * cosA - point.x * sinA
		};
	}
	Vector VecRotZ(Vector point, float angleRad) {
		float sinA = std::sin(angleRad);
		float cosA = std::cos(angleRad);
		return {
			point.x * cosA - point.y * sinA,
			point.y * cosA + point.x * sinA,
			point.z
		};
	}
	Vector VecRelativeTo(Vector point, Vector rotationCenter, Vector eulerDegrees) {
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

	bool Vignette::IntersectsSphere(Vector zone_position, float zone_diameter) const
	{
		// For two spheres to touch, their centers cannot be further apart than the radii of both spheres added together.
		float radius_sum = (diameter + zone_diameter) / 2;
		// We compare squared distances because nobody likes square roots.
		return VecDistanceSquared(position, zone_position) <= radius_sum * radius_sum;
	}

	bool Vignette::IntersectsBox(Vector zone_position, Vector zone_size, Vector zone_rotation) const
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

	bool Vignette::IntersectsEllipsoid(Vector zone_position, Vector zone_size, Vector zone_rotation) const
	{
		// As with boxes, we put the vignette center into the local space of the ellipsoid, so the ellipsoid is axis-aligned and at the origin.
		Vector position_relative = VecRelativeTo(position, zone_position, zone_rotation);

		// We now scale the entire coordinate system by dividing it by the ellipsoid's radii.
		// This means that we're turning the ellipsoid into a unit sphere (meaning its radius is 1), still positioned at the local origin.
		Vector zone_radii = VecDiv(zone_size, 2);
		Vector position_scaled = VecDiv(position_relative, zone_radii);
		float distance_scaled = std::sqrt(VecDistanceToOriginSquared(position_scaled));

		// However, in doing so, we also need to scale the vignette the same way, turning it into an ellipsoid...
		// Sounds weird and pointless, but it's actually the simplest way in this scenario.
		float radius = diameter / 2;
		Vector radii_scaled = VecDiv({ radius, radius, radius }, zone_radii);

		// To avoid dividing by (near-)zero, we just assume that the vignette and the zone overlap if they're in basically the same place.
		if (distance_scaled < 1e-6f)
		{
			return true;
		}
		// Now, we calculate the direction from the zone center to the vignette center as a unit vector.
		Vector dir = VecDiv(position_scaled, distance_scaled);

		// We use that direction to calculate what the "radius" of the scaled vignette is in this direction ("ellipsoidal radius projection").
		float effective_radius = std::sqrt(VecDistanceToOriginSquared(VecMul(radii_scaled, dir)));

		// Now that we have the effective radius, we can check if the scaled vignette's effective radius and the scaled zone's radius (1) overlap,
		// which is done the same way as with two spheres.
		return distance_scaled <= effective_radius + 1;
	}

	bool Vignette::IntersectsCylinder(Vector zone_position, float zone_diameter, float zone_height, Vector zone_rotation) const
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

	bool Vignette::IntersectsRing(Vector zone_position, float zone_diameter, float zone_diameter_inner, float zone_height, Vector zone_rotation) const
	{
		// This could be done more efficiently by reusing some calculations between the two cylinder checks, but this will never get used anyway, so why bother?
		return IntersectsCylinder(zone_position, zone_diameter, zone_height, zone_rotation) && !IntersectsCylinder(zone_position, zone_diameter_inner, zone_height, zone_rotation);
	}
}