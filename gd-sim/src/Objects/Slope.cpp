#include <Slope.hpp>
#include <cmath>
#include <algorithm>
#include <Player.hpp>


Slope::Slope(Vec2D size, std::unordered_map<int, std::string>&& fields) : Block(size, std::move(fields)) {
	auto rot = stod_def(fields[6].c_str());

	bool flipX = atoi(fields[4].c_str()) == 1;
	bool flipY = atoi(fields[5].c_str()) == 1;

	orientation = rot / 90;


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
	if (orientation == 1)
		ang = -ang;
	return ang;
}

double Slope::expectedY(Player const& p) const {
	double ydist = (!p.upsideDown * 2 - 1) * p.size.y * sqrt(pow(tan(angle()), 2) + 1) / 2;
	float posRelative = (size.y / size.x) * (p.pos.x - getLeft());

	if ((angle() > 0) ^ p.upsideDown)
		return getBottom() + std::min(posRelative + ydist, size.y + p.size.y / 2.0);
	else
		return getTop() - std::max(posRelative - ydist, -p.size.y / 2.0);

}

void Slope::calc(Player& p) const {

	if (p.vehicle.type == VehicleType::Wave) {
		p.dead = true;
		return;
	}

	if (gravOrient(p) == 0) {
		if (!touching(p)) {
			p.actions.push_back(+[](Player& p) {
				p.slopeData.slope = {};
				p.slopeData.elapsed = 0.0;
				p.slopeData.snapDown = false;
			});
		}

		if (p.gravBottom(p.prevPlayer()) != getTop()) {
			if (p.upsideDown) {
				p.pos.y = std::min((double)p.pos.y, expectedY(p));
			} else {
				p.pos.y = std::max((double)p.pos.y, expectedY(p));
			}
		}

		if (p.gravBottom(p) == p.gravTop(*this) || (p.gravBottom(p) > p.gravTop(*this) && p.snapData.playerFrame > 0)) {
			double vel = 0.9 * std::min(1.12 / angle(), 1.54) * (size.y * player_speeds[p.speed] / size.x);

			double time = std::clamp(10 * (p.timeElapsed - p.slopeData.elapsed), 0.4, 1.0);

			if (p.vehicle.type == VehicleType::Ball)
				vel *= 0.75;
			if (p.vehicle.type == VehicleType::Ufo)
				vel *= 0.7499;

			vel *= time;

			p.actions.push_back([vel](Player& p) {
				p.velocity = vel;

				p.slopeData.slope = {};
				p.slopeData.elapsed = 0;
				p.slopeData.snapDown = false;
			});
		}
	} else if (gravOrient(p) == 1) {
		if (p.velocity > 0) {
			p.actions.push_back(+[](Player& p) {
				p.slopeData.slope = {};
				p.slopeData.elapsed = 0;
				p.slopeData.snapDown = false;
			});
		}

		if (p.gravBottom(p.prevPlayer()) != getTop() || p.slopeData.snapDown) {
			p.pos.y = std::max(std::min((double)p.pos.y, expectedY(p)), pos.y - p.size.y / 2.);
		}

		if (p.getTop() <= pos.y) {
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
	}
}

void Slope::collide(Player& p) const {
	if (!p.prevPlayer().slopeData.slope && orientation == 1 && p.pos.x - getLeft() < 0 && p.velocity <= 0) {
		p.pos.y = getTop() + p.size.y / 2;
		p.grounded = true;
		return;
	}

	auto pSlope = p.slopeData.slope;
	if (!pSlope || !pSlope->touching(p) || (pSlope->gravOrient(p) == gravOrient(p) && p.grav(expectedY(p)) > p.grav(pSlope->expectedY(p))) || pSlope->id == id) {
		double pAngle = atan((p.velocity * p.dt) / (player_speeds[p.speed] * p.dt));

		bool hasSlope = p.prevPlayer().slopeData.slope.has_value();
		bool projectedHit = orientation == 1 ? (pAngle * 5.0 <= angle()) : (pAngle <= angle());
		bool clip = p.grav(expectedY(p)) >= p.grav(p.pos.y);
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

bool Slope::touching(Player const& p) const {
	if (!Block::touching(p)) {
		return false;
	}

	//std::cout << "GRAV " << gravOrient(p) << std::endl;

	//if (p.upsideDown) return false;

	switch (gravOrient(p)) {
		case 0:
			return p.grav(expectedY(p)) >= p.grav(p.pos.y);
		case 1:
			return p.grav(expectedY(p.prevPlayer())) >= p.grav(p.pos.y);
		case 2:
			return p.grav(expectedY(p)) <= p.grav(p.pos.y);//-(frontBottom.x - pos.x >= frontBottom.y - pos.y);
		case 3:
			return false;//frontBottom.x - pos.x <= frontBottom.y - pos.y;
		default:
			return false;
	}
}
