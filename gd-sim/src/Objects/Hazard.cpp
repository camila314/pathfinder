#include <Hazard.hpp>
#include <Player.hpp>
#include <cmath>

Hazard::Hazard(Vec2D size, std::unordered_map<int, std::string>&& fields) : Object(size, std::move(fields)) {
	// Hazards are processed last, after everything else
	prio = 2;
}

bool Sawblade::touching(Player const& p) const {
	// Hitbox detection for circle

	Vec2D corners[4] = {
		Vec2D(p.getLeft(), p.getBottom()),
		Vec2D(p.getRight(), p.getBottom()),
		Vec2D(p.getRight(), p.getTop()),
		Vec2D(p.getLeft(), p.getTop())
	};

	float radius = size.x;

	for (auto& c : corners) {
		auto diff = c - pos;
		if (std::sqrt(diff.x * diff.x + diff.y * diff.y) <= radius)
			return true;
		c = diff;
	}

	// Check if sawblade is physically inside the hitbox
	if (std::signbit(corners[2].x) != std::signbit(corners[0].x) && std::signbit(corners[2].y) != std::signbit(corners[0].y)) {
		return true;
	}

	return false;
}

void Hazard::collide(Player& p) const {
	p.dead = true;
}
