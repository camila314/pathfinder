#include <Block.hpp>
#include <Slope.hpp>
#include <Level.hpp>
#include <Player.hpp>
#include <cmath>

Block::Block(Vec2D s, std::unordered_map<int, std::string>&& fields) : Object(s, std::move(fields)) {
	// Blocks have a prio of 1, so they are processed later than most other objects.
	prio = 1;

	// No rotation allowed
	if ((int)fabs(rotation) % 180 != 0)
		size = {size.y, size.x};
	rotation = 0;

	// Edge case for this specific block.
	if (fields[1] == "468" && size.y == 5) {
		size.y -= 3.5;
	}
}


/**
 * "Snapping" here refers to what happens when the cube player is
 * holding a button on given block staircases. In order to prevent
 * the player from falling off a block staircase, the X position
 * is manually adjusted by a set amount. The amount depends on
 * the type of staircase, speed, and cube size.
 */
enum class SnapType {
	None,
	BigStair,
	LittleStair,
	DownStair
};


float snapThreshold(Vec2D const& diff, Player const& p) {
	std::array<Vec2D, 3> stairs;
	float threshold;

	switch (p.speed) {
	case 0:
		stairs = { Vec2D(120, -30), Vec2D(90, 30), Vec2D(60, 60) };
		threshold = 1;
		break;
	case 1:
		stairs = { Vec2D(150, -30), Vec2D(p.small ? 90 : 120, 30), Vec2D(90, 60) };
		threshold = 1;
		break;
	case 2:
		stairs = { Vec2D(180, -30), Vec2D(p.small ? 90 : 150, 30), Vec2D(120, 60) };
		threshold = 2;
		break;
	case 3:
		stairs = { Vec2D(225, -30), Vec2D(p.small ? 90 : 180, 30), Vec2D(135, 60) };
		threshold = 2;
		break;
	default:
		stairs = { Vec2D(150, -30), Vec2D(120, 30), Vec2D(90, 60) };
		threshold = 1;
		break;
	}

	for (auto& stair : stairs) {
		if (std::abs(diff.x - stair.x) <= threshold && std::abs(diff.y - stair.y) <= threshold)
			return threshold;
	}

	return 0;
}

void trySnap(Block const& b, Player& p) {
	auto snapData = p.snapData;
	auto diff = b.pos - snapData.object.pos;
	diff.y = p.grav(diff.y);

	if (float threshold = snapThreshold(diff, p); threshold > 0) {
		p.pos.x = std::clamp(
			p.level->getState(snapData.playerFrame).nextPlayer()->pos.x + diff.x,
			p.pos.x - threshold,
			p.pos.x + threshold
		);
	}
}

void Block::collide(Player& p) const {
	// The maximum amount the player can dip below block while still being snapped up
	int clip = (p.vehicle.type == VehicleType::Ufo || p.vehicle.type == VehicleType::Ship) ? 7 : 10;

	// When hitting blue orbs/pads, there is a single frame where block collisions arent done.
	// TODO might be able to be merged with the other pad check
	if (p.upsideDown != p.prevPlayer().upsideDown && !p.gravityPortal)
		return;

	/*
		Going from slope to block means you have to check the bottom of the player
		adjusted for the angle of the slope
	*/
	double bottom = p.gravBottom(p);
	if (p.slopeData.slope) {
		if (p.slopeData.slope->angle() > 0) {
			bottom = bottom + sin(p.slopeData.slope->angle()) * p.size.y / 2;
			clip = 7;

			// Prevent block from catching on slope when it shouldn't
			// TODO this should probably go somewhere else...
			if (p.gravTop(*this) - bottom < 2) {
				return;
			}
		}
	}

	bool padHitBefore = (!p.prevPlayer().grounded && p.prevPlayer().velocity <= 0 && p.velocity > 0);

	if (p.innerHitbox().intersects(*this)) {
		// Hitting block head-on
		p.dead = true;
	} else if (p.vehicle.type != VehicleType::Wave && p.gravTop(*this) - bottom <= clip && (padHitBefore || p.velocity <= 0 || p.gravityPortal)) {
		p.pos.y = p.grav(p.gravTop(*this)) + p.grav(p.size.y / 2);

		// When hitting pads, the next frame will cause the player ot slightly dip into the block
		if (!padHitBefore)
			p.grounded = true;

		// If on a downhill slope and hits a block, the player is no longer on that slope.
		if (p.slopeData.slope && p.slopeData.slope->angle() < 0) {
			p.slopeData.slope = {};
		}

		// X-snapping
		if (p.vehicle.type == VehicleType::Cube) {
			if (!p.prevPlayer().grounded) {
				if (p.snapData.playerFrame > 0 && p.snapData.playerFrame + 1 < p.frame)
					trySnap(*this, p);
			}

			p.snapData.playerFrame = p.level->currentFrame();
			p.snapData.object = *this;
		}
	} else {
		// Ship and ufo can hit the ceiling of a block without dying
		if (p.vehicle.type == VehicleType::Ship || p.vehicle.type == VehicleType::Ufo || p.vehicle.type == VehicleType::Ball) {
			if (p.gravTop(p) - p.gravBottom(*this) <= clip - 1 && p.velocity > 0) {
				p.pos.y = p.grav(p.gravBottom(*this)) - p.grav(p.size.y / 2);
				p.velocity = 0;
			}
		}
	}
}