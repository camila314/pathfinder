/*#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>
#include <deque>

bool calcFlip(bool p) {
	return GameManager::sharedState()->getGameVariable("0010") ? !p : p;
}

bool pathfinderTest(gdr::Replay<>& macro, GJGameLevel* lvl) {
	auto pl = PlayLayer::create(lvl, false, false);
	pl->resetLevel();

	std::deque<gdr::Input> inputs(macro.inputs.begin(), macro.inputs.end());
	bool flipP2 = GameManager::sharedState()->getGameVariable("0010");

	while (!pl->m_hasCompletedLevel && !pl->m_playerDied) {
		if (inputs.size() > 0) {
			auto& input = inputs.front();
			if (input.frame <= macro.frameForTime(pl->m_timePlayed)) {
				pl->handleButton(input.down, input.button, input.player2 ^ flipP2);
				inputs.pop_front();
			}
		}

		pl->update(1/240.);
	}

	return pl->m_playerDied;
}*/