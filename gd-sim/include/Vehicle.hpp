#pragma once

#include <functional>

enum class VehicleType {
	Cube,
	Ship,
	Ball,
	Ufo,
	Wave
};

struct Player;
struct Object;

/// NOT the same as vehicle portal. This class exists to hold vehicle-specific logic.
struct Vehicle {
	VehicleType type;

	/// When the vehicle is changed into this one.
	std::function<void(Player&)> enter;

	/// Ran after everything else, used mainly for vehicles that have ceilings
	std::function<void(Player&)> clamp;

	/// Vehicle-specific movement, done after collisions
	std::function<void(Player&)> update;

	/// How far away the floor and ceiling are from each other, relative to portal
	float bounds;

	static Vehicle from(VehicleType v);
};
