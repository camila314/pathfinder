#pragma once
#include <EffectObject.hpp>

enum class OrbType {
	Yellow,
	Blue,
	Pink,
	Red,
	Green,
	Black,
};

struct Orb : public EffectObject {
	OrbType type;

	Orb(Vec2D size, std::unordered_map<int, std::string>&& fields);
	bool touching(Player const&) const override;
	void collide(Player&) const override;
};