#include <Vehicle.hpp>
#include <Player.hpp>
#include <Object.hpp>
#include <algorithm>
#include <Slope.hpp>
#include <cmath>

Vehicle cube() {
	Vehicle v;
	 v.type = VehicleType::Cube;

	v.enter = +[](Player& p, Object const*, bool n) {
		if (n) {
			if (p.prevPlayer().vehicle.type != VehicleType::Ball)
				p.velocity = p.velocity / 2;

			if (p.prevPlayer().vehicle.type == VehicleType::Ship && p.input)
				p.buffer = true;
		}

		p.ceiling = 999999;
		p.floor = 0;
	};

	v.clamp = +[](Player& p) {
		if (p.velocity < -810)
			p.velocity = -810;

		if (p.gravTop(p.innerHitbox()) >= p.gravCeiling())
			p.dead = true;
	};

	v.update = +[](Player& p) {
		static double accelerations[] = {
			-2747.52,
			-2794.1082, 
			-2786.4,
			-2799.36
		};
		p.acceleration = accelerations[p.speed];

		if (p.gravityPortal && p.grav(p.velocity) > 350 && p.speed > 1) {
			p.acceleration -= 6.48;
		}

		//TODO this is bad
		p.rotation = 0;
		p.rotVelocity = 0;//415.3848;

		bool jump = false;

		if (p.grounded) {
			if (p.input) {
				jump = true;
			} else {
				p.setVelocity(0, true);
			}
			p.rotation = 0;
			p.rotVelocity = 0;
			p.buffer = false;
		}

		if (p.upsideDown && p.input && p.coyoteFrames < 11) {
			jump = true;
			p.buffer = false;
		}

		if (jump) {
			static double jumpHeights[] = {
				573.48,
				603.72,
				616.68,
				606.42,
			};

			if (p.slopeData.slope && p.slopeData.slope->orientation == 0) {
				auto time = std::clamp(10 * (p.timeElapsed - p.slopeData.elapsed), 0.4, 1.0);

				static double slopeHeights[4] = {
					322.345224,
					399.889818,
					497.224926,
					600.643296
				};

				p.setVelocity(0.25 * time * slopeHeights[p.speed] + jumpHeights[p.speed], p.prevPlayer().input);

				p.velocity = std::floor(1000 * p.velocity / 54.) * 54 / 1000.;
				p.grounded = false;
			} else {
				p.setVelocity(jumpHeights[p.speed], p.prevPlayer().input);
				p.grounded = false;
			}
		}
	};

	return v;
}

Vehicle ship() {
	Vehicle v;
	v.type = VehicleType::Ship;

	v.enter = +[](Player& p, Object const* o, bool n) {
		if (n) {
			if (p.prevPlayer().vehicle.type == VehicleType::Ufo || p.prevPlayer().vehicle.type == VehicleType::Wave)
				p.velocity = p.velocity / 4.0;
			else 
				p.velocity = p.velocity / 2.0;
		}

		p.floor = std::max(0., std::ceil((o->pos.y - 180) / 30.)) * 30;
		p.ceiling = p.floor + 300;
	};

	v.clamp = +[](Player& p) {
		p.buffer = false;
		p.velocity = std::clamp(p.velocity, 
			p.small ? -406.566 : -345.6,
			p.small ? 508.248 : 432.0
		);

		if (p.gravTop(p) > p.gravCeiling()) {
			if (p.velocity > 0) {
				p.setVelocity(0, false);
			}
			p.pos.y = p.grav(p.gravCeiling()) - p.grav(p.size.y / 2);
		}
	};

	v.update = +[](Player& p) {
		p.rotation = 0; // TODO ship rotation lmao
		p.rotVelocity = 0;
		p.buffer = false;
	
		if (p.grounded)
			p.setVelocity(0, !p.input);

		constexpr double velcap = 101.541492; // formerly 103.485492

		if (p.input) {
			if (p.velocity <= p.grav(velcap))
				p.acceleration = p.small ? 1643.5872 : 1397.0491;
			else
				p.acceleration = p.small ? 1314.86976 : 1117.64328;
		} else {
			if (p.velocity >= p.grav(velcap))
				p.acceleration = p.small ? -1577.85408 : -1341.1719;
			else
				p.acceleration = p.small ? -1051.8984 : -894.11464;
		}

		if (p.grav(p.pos.y) >= p.gravCeiling()) {
			p.setVelocity(0, false);
		}
	};

	return v;
}

Vehicle ball() {
	Vehicle v;

	v.type = VehicleType::Ball;
	v.clamp = +[](Player& p) {
		if (p.velocity >= 810)
			p.velocity = 810;
		if (p.velocity <= -810)
			p.velocity = -810;

		if (p.grav(p.pos.y) >= p.gravCeiling() && p.velocity > 0) {
			p.setVelocity(0, true);
		}
	};

	v.enter = +[](Player& p, Object const* o, bool n) {
		if (n) {
			p.rotVelocity = 0;

			if (p.input)
				p.vehicleBuffer = true;

			switch (p.prevPlayer().vehicle.type) {
				case VehicleType::Ship:
				case VehicleType::Ufo:
					p.velocity = p.velocity / 2;
					break;
				default: break;
			}
		}
		p.floor = std::max(0., std::ceil((o->pos.y - 150) / 30.)) * 30;
		p.ceiling = p.floor + 240;
	};

	v.update = +[](Player& p) {
		if (!p.prevPlayer().velocityOverride)
			p.acceleration = -1676.46672;

		if (!p.input)
			p.vehicleBuffer = false;

		bool jump = false;

		if (p.grounded) {
			//p.rotVelocity = p.grav(600);
			if (p.input && (p.prevPlayer().buffer || !p.prevPlayer().input || p.vehicleBuffer)) {
				jump = true;
			} else {
				p.setVelocity(0, true);
			}
			p.buffer = false;
		} else if (p.buffer && p.coyoteFrames < (p.upsideDown ? 16 : 1)) {
			jump = true;
		}

		if (jump) {
			static double jumpHeights[] = {
				-172.044007,
				-181.11601,
				-185.00401,
				-181.92601
			};

			p.upsideDown = !p.upsideDown;
			p.setVelocity(jumpHeights[p.speed], p.prevPlayer().buffer || p.vehicleBuffer);
			p.vehicleBuffer = false;
			p.buffer = false;
			p.input = false;
		}
	};

	return v;
}

Vehicle ufo() {
	Vehicle v;

	v.type = VehicleType::Ufo;
	v.enter = +[](Player& p, Object const* o, bool n) {
		if (n) {
			if (p.prevPlayer().vehicle.type == VehicleType::Ship && p.input)
				p.buffer = true;

			p.velocity = p.velocity / (p.prevPlayer().vehicle.type == VehicleType::Ship ? 4 : 2);
		}

		p.floor = std::max(0., std::ceil((o->pos.y - 180) / 30.)) * 30;
		p.ceiling = p.floor + 300;
	};

	v.clamp = +[](Player& p) {
		p.velocity = std::clamp(p.velocity, 
			p.small ? -406.56 : -345.6,
			p.small ? 508.24 : 432.0
		);

		p.input = p.button;

		if (p.gravTop(p) > p.gravCeiling()) {
			if (p.velocity > 0) {
				p.setVelocity(0, false);
			}
			p.pos.y = p.grav(p.gravCeiling()) - p.grav(p.size.y / 2);
		}

	};

	v.update = +[](Player& p) {
		if (p.buffer) {
			p.velocity = std::max(p.velocity, p.small ? 358.992 : 371.034);
			p.velocityOverride = true;
			p.buffer = false;
			p.grounded = false;
		} else {
			if (p.velocity > p.grav(103.485492)) {
				p.acceleration = p.small ? -1969.92 : -1671.84;
			} else {
				p.acceleration = p.small ? -1308.96 : -1114.56;
			}

			if (p.grounded) {
				p.setVelocity(0, true);
			}

			if (p.button)
				p.input = false;
		}
	};

	return v;
}

Vehicle wave() {
	Vehicle v;
	v.type = VehicleType::Wave;
	v.enter = +[](Player& p, Object const* o, bool) {
		p.actions.push_back(+[](Player& p) {
			p.size = Vec2D(10, 10);
		});

		p.floor = std::max(0., std::ceil((o->pos.y - 180) / 30.)) * 30;
		p.ceiling = p.floor + 300;
	};

	v.clamp = +[](Player& p) {
		p.velocity = (p.input * 2 - 1) * player_speeds[p.speed] * (p.small ? 2 : 1);

		if (p.grav(p.pos.y) - p.grav(10) <= p.gravFloor() && !p.input) {
			p.pos.y = p.grav(p.gravFloor()) + p.grav(10);
			p.velocity = 0;
		}

		if (p.grav(p.pos.y) + p.grav(10) >= p.gravCeiling() && p.input) {
			p.velocity = 0;
			p.pos.y = p.grav(p.gravCeiling()) - p.grav(10);
		}
	};
	v.update = +[](Player& p) {
		p.acceleration = 0;
	};

	return v;
}

Vehicle Vehicle::from(VehicleType v) {
	switch (v) {
		case VehicleType::Cube:
			return cube();
		case VehicleType::Ship:
			return ship();
		case VehicleType::Ball:
			return ball();
		case VehicleType::Ufo:
			return ufo();
		case VehicleType::Wave:
			return wave();
	}
}