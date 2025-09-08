#include <Player.hpp>
#include <Level.hpp>
#include <Slope.hpp>
#include <cmath>
#include <climits>

Entity Player::innerHitbox() const {
	return {pos, Vec2D{9, 9}, 0};
}

Entity Player::unrotatedHitbox() const {
	return {pos, size, 0};
}

void Player::setVelocity(double v, bool override) {
	velocityOverride = override;

	// Being small commonly means velocity is 4/5 the original velocity.
	velocity = v * (small ? 0.8 : 1);

	if (v != 0)
		grounded = false;
}

Player const& Player::prevPlayer() const {
	return level->getState(frame - 1);
}

Player const* Player::nextPlayer() const {
	return level->currentFrame() <= frame ? nullptr : &level->getState(frame + 1);
}

/**
 * In Geometry Dash, velocity is stored as 1/54 of distance per second.
 * It is also rounded to the nearest hundredth after (almost) every operation.
 * This function accounts for that rounding
 */
double roundVel(double velocity, bool upsideDown) {
	double nVel = velocity / 54.0 * (upsideDown * 2 - 1);
	double floored = (int)nVel;
	if (nVel != floored) {
		nVel = (double)std::round((nVel - floored) * 1000.0) / 1000.0 + floored;
	}
	return nVel * 54.0 * (upsideDown * 2 - 1);
}

void Player::preCollision(bool pressed) {
	pos.x += player_speeds[(int)speed] * dt;
	pos.y += grav(velocity) * dt;

	frame++;
	timeElapsed += dt;
	grounded = false;
	velocityOverride = false;
	gravityPortal = false;
	roundVelocity = true;

	if (button != pressed) {
		button = pressed;
		input = button;
		buffer = button;
	}

	for (auto& i : actions)
		i(*this);
	actions.clear();

	// Downhill slopes snap you automatically
	if (slopeData.slope && slopeData.slope->orientation == 1) {
		grounded = true;
	}
}



void Player::postCollision() {
	// Size portal only affects hitbox size at the end of frame
	if (small != prevPlayer().small || &prevPlayer() == this) {
		size = small ? (size * 0.6) : (size / 0.6);
	}

	if (gravBottom(*this) <= gravFloor() && !velocityOverride && velocity <= 0 ) {
		pos.y = grav(gravFloor()) + grav(size.y / 2);
		grounded = true;
		snapData.playerFrame = 0;
	}

	// Fell through ceiling, or hit floor
	if (pos.y > 1476.3 || (upsideDown && getBottom() < floor)) {
		dead = true;
		return;
	}

	// Coyote frames 
	if (prevPlayer().gravBottom(*this) > prevPlayer().gravFloor() && upsideDown == prevPlayer().upsideDown && !grounded && velocity <= 0) {
		if (prevPlayer().grounded && !prevPlayer().input)
			coyoteFrames = 0;
		coyoteFrames++;
	} else {
		// Nothing will check for coyote frames this high
		coyoteFrames = INT_MAX;
	}

	vehicle.update(*this);

	if (!velocityOverride) {
		double newVel = velocity + acceleration * dt;

		// Player will fall off blocks a frame faster than expected.
		if (!grounded && prevPlayer().grounded && ((!input && (prevPlayer().button || !button)) || buffer) && prevPlayer().gravBottom(*this) > prevPlayer().gravFloor() && size == prevPlayer().size) {
			pos.y += roundVel(prevPlayer().grav(prevPlayer().acceleration) * dt, prevPlayer().upsideDown) * dt;

			if (gravityPortal && vehicle.type != VehicleType::Ship)
				newVel = -newVel;

			if (velocity == 0)
				newVel += roundVel(prevPlayer().acceleration * dt, upsideDown);
		}
		velocity = newVel;
	}

	// Ball mvoements are not rounded in GD. Probably a bug!
	if (roundVelocity)
		velocity = roundVel(velocity, upsideDown);

	if (slopeData.slope)
		slopeData.slope->calc(*this);

	// Ensure the player hasn't gone beyond the bounds of the vehicle
	vehicle.clamp(*this);
}

Player::Player() :
	Entity({{0, 15}, {30, 30}, 0}), frame(1), timeElapsed(0), dead(false),
	vehicle(Vehicle::from(VehicleType::Cube)),
	ceiling(999999), floor(0), grounded(true),
	coyoteFrames(0), acceleration(0), velocity(0),
	velocityOverride(false), button(false), input(false),
	vehicleBuffer(false), upsideDown(false), small(false),
	speed(1), slopeData({{}, 0, false}), roundVelocity(true) {}







