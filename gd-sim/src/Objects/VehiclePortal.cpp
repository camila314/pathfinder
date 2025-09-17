#include <Portals.hpp>
#include <Player.hpp>

VehiclePortal::VehiclePortal(Vec2D size, std::unordered_map<int, std::string>&& fields) : EffectObject(size, std::move(fields)) {
	switch (atoi(fields[1].c_str())) {
		case 12:
			type = VehicleType::Cube;
			break;
		case 13:
			type = VehicleType::Ship;
			break;
		case 47:
			type = VehicleType::Ball;
			break;
		case 111:
			type = VehicleType::Ufo;
			break;
		case 660:
			type = VehicleType::Wave;
			break;
		default:
			type = VehicleType::Cube;
			break;
		}
}

void VehiclePortal::collide(Player& player) const {
	EffectObject::collide(player);

	if (player.vehicle.type != type) {
		player.size = Vec2D(30, 30) * (player.small ? 0.6 : 1);

		// Going from wave to any other vehicle changes the velocity		
		if (player.vehicle.type == VehicleType::Wave)
			player.velocity *= 0.9;

		player.vehicle = Vehicle::from(type);
		player.vehicle.enter(player);
	}

	player.floor = std::max(0., 30 * std::ceil((pos.y - (player.vehicle.bounds / 2. + 30)) / 30.));
	player.ceiling = player.floor + player.vehicle.bounds;
}