#include <Geode/Geode.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;
using namespace geode::utils::file;

#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <gdr/gdr.hpp>
#include "subprocess.hpp"

std::vector<CCPoint> s_realPoints;
std::vector<CCPoint> s_simPoints;
std::vector<CCPoint> s_inputPoints;
std::string realTxt = "";
static int frames = 0;

class Replay2 : public gdr::Replay<Replay2, gdr::Input> {
 public:
	Replay2() : Replay("GD Sim", "1.0"){}
};


void runTestSim(std::string const& level, std::filesystem::path const& path) {
	std::ifstream macro(path, std::ios::binary);
	std::vector<uint8_t> data((std::istreambuf_iterator<char>(macro)), std::istreambuf_iterator<char>());

	auto replay = Replay2::importData(data, "1.0");
	std::string inputs = "0";
	for (auto& i : replay.inputs) {
		char down = i.down ? '1' : '0';

	    int frameDiff = i.frame - inputs.size();
	    if (frameDiff < 0) {
	        if (frameDiff == -1) {
	            inputs[inputs.size() - 2] = inputs.back();
	            inputs.back() = down;
	            inputs += down;
	        }
	        continue;
	    }

	    char back = inputs.back();
	    for (int j = 0; j < frameDiff; j++) {
	        inputs += back;
	    }
	    inputs += down;
	}

	for (int i = 0; i < 500; ++i)
	    inputs += inputs.back();

	auto lvlFile = Mod::get()->getSaveDir() / ".lvl";
	auto inpFile = Mod::get()->getSaveDir() / ".inp";


	writeString(lvlFile, level).unwrap();
	writeString(inpFile, inputs).unwrap();

	try {
		auto dir = std::filesystem::path(__FILE__).parent_path().parent_path() / "build" / "gd-sim" / "gd-sim-test";
		const auto out = subprocess::check_output({dir, lvlFile, inpFile});
		writeString(Mod::get()->getSaveDir() / "sim.txt", &out.buf[0]).unwrap();


		std::stringstream ss;
		s_simPoints.clear();
		s_inputPoints.clear();
		for (auto& i : out.buf) {
			ss << i;
			if (i == '\n') {
				std::string line = ss.str();
				ss.str("");

				if (line.find("Frame") == 0) {
					int frame;
					float x, y;
					if (sscanf(line.c_str(), "Frame %d X %f Y %f", &frame, &x, &y) == 3) {
						s_simPoints.push_back(ccp(x, y + 105));
					}
				} else if (line.find("Input") == 0) {
					float x, y;
					if (sscanf(line.c_str(), "Input X %f Y %f", &x, &y) == 2) {
						s_inputPoints.push_back(ccp(x, y + 105));
					}
				}
			}
		}
	} catch (const subprocess::CalledProcessError& e) {
		log::error("{}", e.what());
	}
}

class $modify(EditorPauseLayer) {
	void customSetup() {
		EditorPauseLayer::customSetup();

		if (true)
			Loader::get()->queueInMainThread([this]() {
				auto guide = Build(this).intoChildByID("guidelines-menu").collect();
				if (!guide)
					return;
				Build<CCSprite>::createSpriteName("GJ_createLinesBtn_001.png")
					.scale(0.7)
					.intoMenuItem([]() {
						CCDrawNode* node = (CCDrawNode*)LevelEditorLayer::get()->m_objectLayer->getChildByID("pathfind-node");
						node->clear();
						CCPoint start = ccp(0, 105);
						for (auto& i : s_simPoints) {
							node->drawSegment(start, i, 1, ccc4f(0, 1, 0, 1));
							start = i;
						}

						start = ccp(0, 105);
						for (auto& i : s_realPoints) {
							node->drawSegment(start, i, 1, ccc4f(1, 0, 0, 1));
							start = i;
						}

						bool k = false;
						for (auto& i : s_inputPoints) {
							node->drawDot(i, 2, ccc4f(k ? 1 : 0, 0.5, k ? 0 : 1, 1));
							k = !k;
						}
					})
					.parent(guide);

				guide->updateLayout();
			});
	}
};

class $modify(EditLevelLayer) {
    bool init(GJGameLevel* p0) {
        EditLevelLayer::init(p0);

        auto btn = Build<BasedButtonSprite>::create(
            CCSprite::create("pathfinder.png"_spr),
            BaseType::Circle,
            4,
            2
        ).scale(0.8);

        btn->setTopRelativeScale(1.4);

        btn.intoMenuItem([this]() {
                auto lvlString = ZipUtils::decompressString(m_level->m_levelString, true, 0);
            	pick(PickMode::OpenFile, {}).listen([lvlString](auto path) {
            		if (*path)
            			runTestSim(lvlString, path->unwrap());
            	}, [](auto) {}, []() {});
        }).id("pathfinder-debug-button")
          .intoNewParent(CCMenu::create())
          .parent(this)
          .id("pathfinder-debug-menu")
          .matchPos(getChildByIDRecursive("delete-button"))
          .move(-45, -50);


        return true;
    }
};

class $modify(LevelEditorLayer) {
	bool init(GJGameLevel* lvl, bool p1) {
		LevelEditorLayer::init(lvl, p1);
		(void)file::writeString(Mod::get()->getSaveDir() / "real.txt", realTxt);

		if (true) {
			auto b = CCDrawNode::create();
			b->setID("pathfind-node");
			m_objectLayer->addChild(b);

			CCPoint start = ccp(0, 105);
			for (auto& i : s_realPoints) {
				b->drawSegment(start, i, 1, ccc4f(1, 0, 0, 1));
				start = i;
			}
		}

		return true;
	}
};

class $modify(GJBaseGameLayer) {
	void processCommands(float dt) {
		if (true) {
			GJBaseGameLayer::processCommands(1 / 240.);

			if (PlayLayer::get()) {
				static double prevVel = 0;
				static double prevX = 0;
				static double prevXVel = 0;	

				double vel = m_player1->m_yVelocity * 60 * 0.9 * (m_player1->m_isUpsideDown ? -1 : 1);
				log::info("{}", reference_cast<long>(m_player1->m_yVelocity));
				double xvel = (m_player1->getPositionX() - prevX);

				auto dat = fmt::format("Frame {} X {:.8f} Y {:.8f} Vel {:.8f} Accel {:.8f}", frames, m_player1->getPositionX(), m_player1->getPositionY() - 105, vel, (vel - prevVel) * 240);
				log::info("{}", dat);
				realTxt += dat + "\n";

				frames++;
				prevVel = vel;
				prevXVel = xvel;
				prevX = m_player1->getPositionX();

				s_realPoints.push_back(m_player1->getPosition());
			}
		} else {
			GJBaseGameLayer::processCommands(dt);
		}
	}
};
class $modify(PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		realTxt = "";
		s_realPoints.clear();
		frames = 0;
	}
};
