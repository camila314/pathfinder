#include <Slope.hpp>
#include <cmath>
#include <algorithm>
#include <Player.hpp>

/// Slopes are possibly the most complicated object. This is still very unfinished!

Slope::Slope(Vec2D size, std::unordered_map<int, std::string>&& fields) : Block(size, std::move(fields)) {
	auto rot = stod_def(fields[6].c_str());

	bool flipX = atoi(fields[4].c_str()) == 1;
	bool flipY = atoi(fields[5].c_str()) == 1;

	orientation = rot / 90;

	// Slope orientation matters a lot lot
	if (flipX && flipY)
		orientation += 2;
	else if (flipX)
		orientation += 1;
	else if (flipY)
		orientation += 3;

	orientation = orientation % 4;
	if (orientation < 0) orientation += 4;

	rotation = 0;
}

bool Slope::isFacingUp() const {
	return orientation < 2;
}

int Slope::gravOrient(Player const& p) const {
	int orient = orientation;

	if (p.upsideDown) {
		if (orient == 3)
			orient = 0;
		else if (orient == 2)
			orient = 1;
		else if (orient == 0)
			orient = 3;
		else if (orient == 1)
			orient = 2;
	}

	return orient;
}

double Slope::angle() const {
	auto ang = std::atan(size.y / size.x);
	// Downhill slopes 
	if (orientation == 1 || orientation == 3)
		ang = -ang;
	return ang;
}

double Slope::expectedY(Player const& p) const {
	double ydist = (isFacingUp() ? 1 : -1) * (p.size.y / 2.f) / cosf(angle());
	float posRelative = (size.y / size.x) * (p.pos.x - getLeft());

	// Uphill vs downhill
	if (angle() > 0)
		return getBottom() + std::min(posRelative + ydist, size.y + p.size.y / 2.0);
	else
		return getTop() - std::min(posRelative - ydist, size.y + p.size.y / 2.0);
}

/// See Slope.hpp for why this is a separate function
void Slope::calc(Player& p) const {
	if (gravOrient(p.prevPlayer()) == 0) {
		// Regular uphill slope

		// Coyote frame for slopes must be taken into account
		if (!touching(p)) {
			p.actions.push_back(+[](Player& p) {
				p.slopeData.slope = {};
				p.slopeData.elapsed = 0.0;
				p.slopeData.snapDown = false;
			});
		}

		//  If player isn't on top already, use expectedY to snap player
		if (p.gravBottom(p.prevPlayer()) != getTop()) {
			if (p.prevPlayer().upsideDown) {
				p.pos.y = std::min((double)p.pos.y, expectedY(p));
			} else {
				p.pos.y = std::max((double)p.pos.y, expectedY(p));
			}
		}

		// When you're on top of the slope you will be ejected
		if (p.grounded && (p.gravBottom(p) == p.gravTop(*this) || (p.gravBottom(p) > p.gravTop(*this) && p.snapData.playerFrame > 0))) {
			
			// Rob's algorithm for slope ejection velocity. So goofy!
			double vel = 0.9 * std::min(1.12 / p.grav(angle()), 1.54) * (size.y * player_speeds[p.speed] / size.x);
			double time = std::clamp(10 * (p.timeElapsed - p.slopeData.elapsed), 0.4, 1.0);

			if (p.vehicle.type == VehicleType::Ball || p.vehicle.type == VehicleType::Ship)
				vel *= 0.75;
			if (p.vehicle.type == VehicleType::Ufo)
				vel *= 0.7499; // I have no justification for this. It just works

			vel *= time;

			// Gotta eject on the next frame
			p.actions.push_back([vel](Player& p) {
				p.velocity = roundVel(vel, p.upsideDown);

				p.slopeData.slope = {};
				p.slopeData.elapsed = 0;
				p.slopeData.snapDown = false;
			});
		}
	} else if (gravOrient(p.prevPlayer()) == 1) {
		// Downhill regular slope

		// Velocity up means you're not on slope anymore
		if (p.velocity > 0) {
			p.actions.push_back(+[](Player& p) {
				p.slopeData.slope = {};
				p.slopeData.elapsed = 0;
				p.slopeData.snapDown = false;
			});
		}

		// Snap to expected Y just like uphill
		if (p.gravBottom(p.prevPlayer()) != getTop() || p.slopeData.snapDown) {
			p.pos.y = std::max(std::min((double)p.pos.y, expectedY(p)), pos.y - p.size.y / 2.);
		}

		// Ejections, but downwards!
		if (p.getTop() <= pos.y) {
			// TODO use the actual algorithm because there are probably wrong
			static double falls[4] = {
				226.044054,
				280.422108,
				348.678108,
				421.200108
			};

			double vel = -falls[p.speed] * (size.y / size.x);
			p.velocity = 0;
			p.actions.push_back([vel](Player& p) {
				p.velocity = vel;
				p.slopeData.slope = {};
				p.slopeData.elapsed = 0;
				p.slopeData.snapDown = false;
			});
		}
	} else {
		if (!touching(p)) {
			p.actions.push_back(+[](Player& p) {
				p.slopeData.slope = {};
				p.slopeData.elapsed = 0.0;
				p.slopeData.snapDown = false;
			});
		}
	}
}

void Slope::collide(Player& p) const {
	// No slope calculations for you!
	if (p.vehicle.type == VehicleType::Wave) {
		p.dead = true;
		return;
	}

	// When you hit a downhill slope before your center hits the leftmost side, it's treated like a block
	if (!p.prevPlayer().slopeData.slope && orientation == 1 && p.pos.x - getLeft() < 0 && p.velocity <= 0) {
		p.pos.y = getTop() + p.size.y / 2;
		p.grounded = true;
		return;
	}

	// Current (or previous) slope
	auto pSlope = p.slopeData.slope;

	/*
		If stored slope data is current slope, or there is no stored,
		or you're no longer touching the previous slope.
	*/
	if (!pSlope || !pSlope->touching(p) || (pSlope->gravOrient(p) == gravOrient(p) && p.grav(expectedY(p)) > p.grav(pSlope->expectedY(p))) || pSlope->id == id) {
		double pAngle = atan((p.prevPlayer().velocity * p.dt) / (player_speeds[p.speed] * p.dt));

		bool hasSlope = p.prevPlayer().slopeData.slope.has_value();

		//  Is player traveling at the right angle to contact the slope
		bool projectedHit = orientation == 1 ? (pAngle * 5.0 <= angle()) : (pAngle <= angle());

		// Downhill slopes attach you to the slope faster uphill
		//float expAdjust = expectedY(p)  + (angle() > 0 ? 0 : 2);
		bool clip = true;//isFacingUp() ? (p.grav(expAdjust) >= p.grav(p.pos.y)) : (p.grav(expAdjust) <= p.grav(p.pos.y));

		// Downhill slopes snap you down
		bool snapDown = orientation == 1 && p.velocity > 0 && p.pos.x - getLeft() > 0;

		if (hasSlope ? p.velocity <= 0 : projectedHit & clip || snapDown) {
			p.grounded = true;
			p.slopeData.slope = *this;

			if (snapDown && !hasSlope) {
				p.velocity = 0;
				p.pos.y = getTop() + p.size.y / 2;
				p.slopeData.snapDown = true;
			}

			if (!p.slopeData.elapsed) 
				p.slopeData.elapsed = p.prevPlayer().timeElapsed;
		}
	}
}

void SlopeHazard::collide(Player& p) const {
	p.dead = true;
}
double SlopeHazard::expectedY(Player const& p) const {
	// Hazardous slopes have slightly larger hitboxes
	return Slope::expectedY(p) + (orientation > 1 ? -4 : 4);
}

bool Slope::touching(Player const& p) const {
	if (!Block::touching(p) || (p.vehicle.type == VehicleType::Cube && p.gravTop(p) - p.gravBottom(*this) < 16)) {
		return false;
	}

	// TODO finish the last two.
	switch (orientation) {
		case 0:
			return expectedY(p) > p.pos.y;
		case 1:
			return expectedY(p) > p.pos.y;
		case 2:
			return expectedY(p) < p.pos.y;//-(frontBottom.x - pos.x >= frontBottom.y - pos.y);
		case 3:
			return expectedY(p) < p.pos.y;//frontBottom.x - pos.x <= frontBottom.y - pos.y;
		default:
			return false;
	}
}
