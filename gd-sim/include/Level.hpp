#pragma once
#include <Object.hpp>
#include <Player.hpp>
#include <vector>
#include <set>

static std::set unsupportedObjects = {
    // gameplay tab
     286, // dual portal
     660, // wave portal
     745, // robot portal
     747, // teleport portal (blue)
     748, // teleport portal (orange)
     660, // wave portal
    1022, // green orb
    1330, // black orb
    1331, // spider portal
    1332, // red pad
    1333, // red orb
    1334, // 4x speed
    1594, // toggle orb
    1704, // dash orb
    1751, // gravity dash orb
    1755, // d modifier block
    1813, // j modifier block
    1829, // s modifier block
    1859, // h modifier block
    1933, // swing portal
    2063, // checkpoint (in case some nerd wants to use it in classic mode)
    2069, // force block (square)
    2866, // f modifier block
    2926, // flip portal
    3027, // teleport orb
    3645, // force block (circle)

    // animations and particles tab
    1327, // small monster
    1328, // large monster
    1584, // bat monster
    2012, // spikeball monster

    // triggers tab
     901, // move trigger
    1049, // toggle trigger
    1268, // spawn trigger
    1347, // follow trigger
    1346, // rotate trigger
    1585, // animate trigger
    1616, // stop trigger
    2067, // scale trigger
    3003, // keyframe trigger
    1814, // follow player y trigger
    3016, // advanced follow trigger
    3660, // edit advanced follow trigger
    3661, // re-target advanced follow trigger
    3032, // keyframe trigger
    3006, // area move trigger
    3007, // area rotate trigger
    3008, // area scale trigger
    3011, // edit area move trigger
    3012, // edit area rotate trigger
    3013, // edit area scale trigger
    1595, // touch trigger
    1611, // count trigger
    1811, // instant count trigger
    1817, // pickup trigger
    3614, // time trigger
    3615, // time event trigger
    3617, // time control trigger
    3619, // item edit trigger
    3620, // item compare trigger
    3641, // item persist trigger
    1912, // random trigger
    2068, // advanced random trigger
    3607, // sequence trigger
    3618, // reset item trigger
    1917, // reverse trigger
    2900, // arrow trigger
    3604, // event trigger
    1935, // timewarp trigger
    1815, // collision trigger
    3609, // instant collision trigger
    1812, // on death trigger
    1932, // player control trigger
    2899, // options trigger
    2066, // gravity trigger
    3022, // teleport trigger
    3017, // area move trigger
    3018, // area rotate trigger
    3019, // area scale trigger

    // circular spinny things tab
    1702, // spiked circle hazard

    // hazards tab
    3610, // damage square
    3611, // damage circle
};

class Level {
	void initLevelSettings(std::string const& lvlSettings, Player& player);
 public:
	std::vector<Player> gameStates;
	size_t objectCount = 0;
	std::vector<std::vector<ObjectContainer>> sections;
	float length = 0.0;
	bool mayBeUnsupported = false; // for version checking

 	static constexpr uint32_t sectionSize = 100;
 	bool debug = false;

 	Level(std::string const& lvlString);
 	Player& runFrame(bool pressed, float dt = 1/240.);
 	void rollback(int frame);
 	int currentFrame() const;
 	Player const& getState(int frame) const;
 	Player& latestFrame();
};
