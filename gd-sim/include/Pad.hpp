#pragma once
#include <EffectObject.hpp>

enum class PadType {
	Yellow,
	Blue,
	Pink,
	Red,
};

struct Pad : public EffectObject {
	PadType type;

	Pad(Vec2D size, std::unordered_map<int, std::string>&& fields);
	void collide(Player&) const override;
};