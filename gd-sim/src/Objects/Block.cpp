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

/// `diff` refers to the coordinate difference between the current and previous touched block
SnapType snapType(Vec2D const& diff, Player const& p) {
	if (p.speed == 0 && !p.small) {
		if (diff == Vec2D(90, 30)) return SnapType::LittleStair;
		if (diff == Vec2D(60, 60)) return SnapType::BigStair;
		if (diff == Vec2D(120, -30)) return SnapType::DownStair;
	} else if (p.speed == 1) {
		if (diff == Vec2D(90, 60) || (p.small && diff == Vec2D(90, 30))) return SnapType::BigStair;
		if (diff == Vec2D(120, 30) && !p.small) return SnapType::LittleStair;
		if (diff == Vec2D(150, -30)) return SnapType::DownStair;
	} else if (p.speed == 2 && !p.small) {
		if (diff == Vec2D(120, 60)) return SnapType::BigStair;
		if (diff == Vec2D(150, 30)) return SnapType::LittleStair;
	} else if (p.speed == 3 && !p.small) {
		if (diff == Vec2D(210, -30)) return SnapType::DownStair;
	}

	return SnapType::None;
}

void trySnap(Block const& b, Player& p) {
	auto snapData = p.snapData;
	auto diff = b.pos - snapData.object.pos;
	diff.y = p.grav(diff.y);

	auto snapPlayer = p.level->getState(snapData.playerFrame);

	auto held = snapPlayer.input;
	bool small = snapPlayer.small;

	bool onEdge = 
		(snapPlayer.getRight() - snapData.object.getLeft() < 1) || 
		(snapPlayer.nextPlayer()->getLeft() - snapData.object.getRight() > -1);
	
	float samePos = snapPlayer.nextPlayer()->pos.x + diff.x;

	/*
		There are three main types of snapping: adding by 1 or 2, subtracting by 1 or 2, and
		adjusting the x position based on the previous x position when touching the last block.
	*/
	switch (snapType(diff, p)) {
		case SnapType::BigStair:
			if ((!small && held) || snapPlayer.getRight() - snapData.object.getLeft() < 1)
				p.pos.x = samePos;
			else
				p.pos.x += std::max(1, p.speed);
			break;
		case SnapType::LittleStair:
			if (p.speed == 1) {
				if (held && onEdge)
					p.pos.x = samePos;
				else 
					p.pos.x += 1;
			} else {
				if (held)
					p.pos.x -= (p.speed == 0 ? 1 : 2);
				else
					p.pos.x = samePos;
			}
			break;
		case SnapType::DownStair:
			if (held && !small && p.speed < 3)
				p.pos.x = samePos;
			else {
				if (p.speed < 2)
					p.pos.x += 1;
				else
					p.pos.x += 2;
			}
			break;
		default:
			break;
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
				std::cout << "wow whats this?\n";
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