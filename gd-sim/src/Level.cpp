#include <sstream>
#include <iomanip>
#include <Level.hpp>

void Level::initLevelSettings(std::string const& lvlSettings, Player& player) {
	std::unordered_map<std::string, std::string> obj;

	std::stringstream ss2(lvlSettings);
	std::string k, v;
	while (std::getline(ss2, k, ',')) {
		std::getline(ss2, v, ',');
		obj[k] = v;
	}

	auto get_or = [&obj](std::string const& key, std::string const& def) {
		if (auto it = obj.find(key); it != obj.end())
			return it->second.c_str();
		return def.c_str();
	};

	player.speed = atoi(get_or("kA4", "0"));
	if (player.speed == 0)
		player.speed = 1;
	else if (player.speed == 1)
		player.speed = 0;

	player.small = atoi(get_or("kA3", "0"));
	player.upsideDown = atoi(get_or("kA11", "0"));
	player.vehicle = Vehicle::from(static_cast<VehicleType>(atoi(get_or("kA2", "0"))));
}

Level::Level(std::string const& lvlString) {
	// split by ';'
	std::stringstream ss(lvlString);
	std::string objstr;
	bool first = true;
	auto player = Player();

	while (std::getline(ss, objstr, ';')) {
		if (first) {
			// level settings
			initLevelSettings(objstr, player);
			first = false;
			continue;
		}

		std::unordered_map<int, std::string> obj;

		std::stringstream ss2(objstr);
		std::string k, v;
		while (std::getline(ss2, k, ',')) {
			std::getline(ss2, v, ',');
			if (atoi(k.c_str()) > 0)
				obj[atoi(k.c_str())] = v;
		}

		if (auto ob_o = Object::create(std::move(obj))) {
			auto ob = ob_o.value();
			ob->id = objectCount++;

			size_t sectionPos = std::max(.0f, ob->pos.x / sectionSize);
			if (sectionPos >= sections.size())
				sections.resize(sectionPos + 1);
			sections[sectionPos].push_back(ob);

			if (ob->pos.x > length)
				length = ob->pos.x + 100;
		} else if (!mayBeUnsupported) {
			mayBeUnsupported = true; // only set the bool once. if ob_o is invalid it is unsupported
		}
	}

	player.level = this;
	gameStates.push_back(player);
}

Player& Level::runFrame(bool pressed, float dt) {
	Player p = gameStates.back();
	if (p.dead)
		return gameStates.back();

	p.dt = dt;
	p.preCollision(pressed);

	size_t sectionIdx = std::min(std::max(0, (int)(p.pos.x / sectionSize)), (int)sections.size() - 1);
	auto prevSection = &sections[sectionIdx == 0 ? 0 : sectionIdx - 1];
	auto currSection = &sections[sectionIdx];
	auto nextSection = &sections[sectionIdx + 1 >= sections.size() - 1 ? sections.size() - 1 : sectionIdx + 1];

	
	std::vector<ObjectContainer>* sections[3] = { prevSection, nullptr, nullptr };
	if (&currSection != &prevSection)
		sections[1] = currSection;
	if (&nextSection != &currSection)
		sections[2] = nextSection;

	std::vector<ObjectContainer> blocks;
	std::vector<ObjectContainer> hazards;
	blocks.reserve(100);
	hazards.reserve(100);

	size_t numCollisions = 0;

	for (auto section : sections) {
		if (section == nullptr) continue;
		for (auto& o : *section) {
			if (p.dead) break;
			if (o->prio == 1)
				blocks.push_back(o);
			else if (o->prio == 2)
				hazards.push_back(o);
			else if (o->touching(p)) {
				++numCollisions;
				o->collide(p);
			}
		}
	}

	for (int i = blocks.size() - 1; i >= 0; --i) {
		if (p.dead) break;
		auto& b = blocks[i];
		if (b->touching(p)) {
			++numCollisions;
			b->collide(p);
		}
	}

	for (auto& h : hazards) {
		if (p.dead) break;
		if (h->touching(p)) {
			++numCollisions;
			h->collide(p);
		}
	}

	if (!p.dead)
		p.postCollision();

	if (debug) {
		std::cout << "Frame " << gameStates.size() << std::fixed << std::setprecision(8)
				  << " X " << p.pos.x << " Y " << p.pos.y - 15 << " Vel " << p.velocity
				  << " Accel " << p.acceleration << " G " << p.grounded << " Coll " << numCollisions
				  << std::endl;

		if (p.button != gameStates.back().button) {
			std::cout << "Input X " << p.pos.x << " Y " << p.pos.y - 15 << std::endl;
		}
	}

	gameStates.push_back(p);
	return gameStates.back();
}


void Level::rollback(int frame) {
	gameStates.resize(frame > 0 ? frame : 1);
}

int Level::currentFrame() const {
	return gameStates.size();
}

Player const& Level::getState(int frame) const {
	if (frame == 0)
		return gameStates[0];
	if (gameStates.size() < frame)
		return gameStates.back();
	return gameStates[frame - 1];
}

Player& Level::latestFrame() {
	return gameStates.back();
}
