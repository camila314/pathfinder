#pragma once

#undef small
#include <util.hpp>
#include <Vehicle.hpp>
#include <Slope.hpp>
#include <vector>
#include <functional>
#include <optional>


/// Player X velocity per speed
inline double player_speeds[5] = {
	251.16007972276924,
	311.580093712804,
	387.42014039710523,
	468.0001388338566,
	576.00020058307177
};

/// Used in player rotation. Similar to m_playerSpeed member variable
inline float player_speedmults[5] = {
	0.7,
	0.9,
	1.1,
	1.3,
	1.6	
};

double roundVel(double velocity, bool upsideDown);

struct Object;
class Level;
struct Slope;

/**
 * The main player. This contains the entire player state and is the only thing that changes each frame.
 */
struct Player : public Entity {
	Vehicle vehicle;
	Level* level;

	double timeElapsed;
	double acceleration;
	double velocity;

	/// See util.hpp for what cow_set is
	cow_set<int> usedEffects;

	/// Actions will be ran at the beginning of every frame.
	std::vector<std::function<void(Player&)>> actions;

	/// Slopes have special collision rules. See Slope.cpp for more information.
	struct {
		std::optional<Slope> slope;

		/// Time on slope
		double elapsed;

		/// When colliding with a downhill slope with a positive velocity.
		bool snapDown;
	} slopeData;

	/// X-snapping. See Block.cpp for more information
	struct {
		Entity object;
		int playerFrame = 0;
	} snapData;

	float ceiling;
	float floor;
	float dt;

	/// Some vehicles have coyote frames for valid inputs
	unsigned int coyoteFrames;

	int speed;
	int frame;

	bool dead;
	bool grounded;

	/**
	 * Under normal circumstances, acceleration is applied to the velocity
	 * at the end of a frame. When this field is set, acceleration will not
	 * be applied at the end of the frame. Mainly set via `setVelocity`
	 */
	bool velocityOverride;

	/**
	 * `button` vs `input`: Button always refers to whether a click was applied; input can be disabled
	 * for niche circumstances where no other normal operations are allowed despite a click.
	 */
	bool button, input;

	/// If a click is being buffered. Used for things like orb clicks.
	bool buffer;
	/**
	 * In the ball vehicle, holding a click while transitioning into another vehicle will cause a
	 * buffered input, despite things like orbs not buffering in the same way.
	 */
	bool vehicleBuffer;

	bool upsideDown;
	bool small;

	/// Entering a gravity portal can cause the next frame to have certain edge cases
	bool gravityPortal;

	/// Whether velocity at the end of the frame is to be rounded (typically used by ball)
	bool roundVelocity;

	Player();

	void preCollision(bool input);
	void postCollision();

	
	Entity unrotatedHitbox() const;

	/// Inner hitbox of {9, 9} that is used mainly for blocks
	Entity innerHitbox() const;

	Player const& prevPlayer() const;
	Player const* nextPlayer() const;

	/// Values relative to player gravity.
	template <typename T>
	T grav(T value) const { return upsideDown ? -value : value; }
	inline float gravBottom(Entity const& e) const { return upsideDown ? -e.getTop() : e.getBottom(); }
	inline float gravTop(Entity const& e) const { return upsideDown ? -e.getBottom() : e.getTop(); }
	inline float gravFloor() const { return upsideDown ? -ceiling : floor; }
	inline float gravCeiling() const { return upsideDown ? -floor : ceiling; }

	void setVelocity(double v, bool override=false);
};