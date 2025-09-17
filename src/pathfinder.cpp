#include <set>
#include <Level.hpp>
#include <random>
#include <gdr/gdr.hpp>
#include "pathfinder.hpp"

class Replay2 : public gdr::Replay<Replay2, gdr::Input<"">> {
 public:
	Replay2() : Replay("Pathfinder", 1){}
};

struct Level2 : public Level {
	bool press = false;
	float highestY = 0;
	using Level::Level;

	Level2(std::string const& lvlString) : Level(lvlString) {
		// Find highest y
		for (auto& i : sections) {
			for (auto& j : i) {
				highestY = std::max(highestY, j->pos.y);
			}
		}
	}
};

bool isLevelEnd(Level2& lvl) {
	return lvl.latestState().pos.x >= lvl.length;
}

int tryInputs(Level2& lvl, std::set<uint16_t> inputs) {
	auto frame = lvl.currentFrame();
	auto press_before = lvl.press;

	while (!inputs.empty() && !lvl.gameStates.back().dead) {
		if (inputs.contains(lvl.currentFrame())) {
			lvl.press = !lvl.press;
			inputs.erase(lvl.currentFrame());
		}

		lvl.runFrame(lvl.press);
	}
	int final = lvl.currentFrame();
	float lastX = lvl.latestState().pos.y;
	float lastY = lvl.latestState().pos.y;

	lvl.rollback(frame);
	lvl.press = press_before;

	if (lastX < lvl.length && (lastY > 1300 || lastY < 0))
		return 0;

	return final;
}

std::vector<uint8_t> pathfind(std::string const& lvlString, std::atomic_bool& stop, std::function<void(double)> callback) {
	Level2 lvl(lvlString);

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> dist(0, 999);

	int trueBest = 0;
	int fail = 1;
	int numAway = 1000;

	Level2 lvlBest = lvl;

	while (lvl.gameStates.back().pos.x < lvl.length) {
		auto frame = lvl.currentFrame();

		std::set<uint16_t> bestInputs;
		int bestFrame = frame;

		constexpr int iterations = 300; //30
		for (int i = 0; i < iterations; i++) {

			std::set<uint16_t> inputs;
			for (int i = 0; i < 30; i++) {
				inputs.insert(frame + dist(rng));
			}

			int nf = tryInputs(lvl, inputs);

			if (nf > bestFrame) {
				bestFrame = nf;
				bestInputs = inputs;
				if (bestFrame - frame > 500 && fail < 1000)
					break;
			}
		}

		if (bestFrame == frame) {
			lvl.rollback(std::max(std::max(frame - fail, trueBest - numAway), 1));

			fail += 5;

			if (fail > numAway + 1000) {
				numAway += 1000;
				fail = 1;

				if (numAway > 10000) {
					numAway = 1000;
					trueBest = 0;
					lvl.rollback(1);
				}
			} else if (fail > 100) {
				fail += 50;
			}
		} else {
			for (int i = frame; i < bestFrame - (bestFrame - frame) / 1.5; ++i) {
				if (bestInputs.contains(i)) {
					lvl.press = !lvl.press;
				}
				lvl.runFrame(lvl.press);
			}
		}

		if (lvl.currentFrame() > trueBest) {
			trueBest = lvl.currentFrame();
			fail = 0;
			numAway = 1000;
		}
		if (lvl.currentFrame() > lvlBest.currentFrame()) {
			lvlBest = lvl;
		}

		if (callback)
			callback(std::min((lvl.latestState().pos.x / lvl.length) * 100, 100.0f));
		//std::cout << "\rStatus: " << std::min((lvl.latestFrame().pos.x / lvl.length) * 100, 100.0f) << "%            " << std::flush;

		if (stop)
			break;
	}

	Replay2 output;
	for (auto& i : lvlBest.gameStates) {
	    if (i.frame > 1 && i.button != i.prevPlayer().button)
	        output.inputs.push_back(gdr::Input(i.frame, 1, false, i.button));
	}

	return output.exportData().unwrapOr({});
}
