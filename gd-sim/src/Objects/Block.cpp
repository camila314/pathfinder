#include <Block.hpp>
#include <Slope.hpp>
#include <Level.hpp>
#include <Player.hpp>
#include <cmath>

Block::Block(Vec2D s, std::unordered_map<int, std::string>&& fields) : Object(s, std::move(fields)) {
	prio = 1;

	if ((int)fabs(rotation) % 180 != 0)
		size = {size.y, size.x};
	rotation = 0;

	if (fields[1] == "468" && size.y == 5) {
		size.y -= 3.5;
	}
}


enum class SnapType {
	None,
	BigStair,
	LittleStair,
	DownStair
};

SnapType snapType(Vec2D const& diff, Player const& p) {
	if (p.speed == 0 && !p.small) {
		if (diff == Vec2D(90, 30)) return SnapType::LittleStair;
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

	switch (snapType(diff, p)) {
		case SnapType::BigStair:
			if ((!small && held) || snapPlayer.getRight() - snapData.object.getLeft() < 1)
				p.pos.x = samePos;
			else
				p.pos.x += p.speed;
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
	int clip = (p.vehicle.type == VehicleType::Ufo || p.vehicle.type == VehicleType::Ship) ? 7 : 10;

	if (p.upsideDown != p.prevPlayer().upsideDown && !p.gravityPortal)
		return;

	double bottom = p.gravBottom(p);
	if (p.slopeData.slope) {
		bottom = bottom + sin(p.slopeData.slope->angle()) * p.size.y / 2;
		clip = 7;
		if (p.gravTop(*this) - bottom < 2)
			return;
	}


	bool padHitBefore = (!p.prevPlayer().grounded && p.prevPlayer().velocity <= 0 && p.velocity > 0);

	if (p.innerHitbox().intersects(*this)) {
		p.dead = true;
	} else if (p.gravTop(*this) - bottom <= clip && (padHitBefore || p.velocity <= 0 || p.gravityPortal)) {
		p.pos.y = p.grav(p.gravTop(*this)) + p.grav(p.size.y / 2);
		if (!padHitBefore)
			p.grounded = true;

		if (p.vehicle.type == VehicleType::Cube) {
			if (!p.prevPlayer().grounded) {
				if (p.snapData.playerFrame > 0 && p.snapData.playerFrame + 1 < p.frame)
					trySnap(*this, p);
			}

			p.snapData.playerFrame = p.level->currentFrame();
			p.snapData.object = *this;
		}
	} else {
		if (p.vehicle.type == VehicleType::Ship || p.vehicle.type == VehicleType::Ufo) {
			if (p.gravTop(p) - p.gravBottom(*this) <= clip && p.velocity > 0) {
				p.pos.y = p.grav(p.gravBottom(*this)) - p.grav(p.size.y / 2);
				p.velocity = 0;
			}
		}
	}
}