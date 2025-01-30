#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <UIBuilder.hpp>
#include "pathfinder.hpp"
#include <future>

using namespace geode::prelude;
using namespace geode::utils::file;

class PathfinderNode : public CCLayerColor {
    std::atomic_bool m_stop = false;
    std::atomic<double> m_progress = 0;
    std::future<std::vector<uint8_t>> m_result;
    std::string m_levelName;
public:
    static PathfinderNode* create(std::string const& levelName, std::string const& lvlString) {
        auto node = new PathfinderNode();
        if (node && node->init(levelName, lvlString)) {
            node->autorelease();
            return node;
        }
        CC_SAFE_DELETE(node);
        return nullptr;
    }

    ~PathfinderNode() {
        m_stop = true;
        if (m_result.valid())
            m_result.get();
    }

    void finalize(std::vector<uint8_t> macro) {
        getChildByIDRecursive("cancel")->setVisible(false);
 
        Build<ButtonSprite>::create("Export", "bigFont.fnt", "GJ_button_01.png")
            .intoMenuItem([this, macro]() {
                auto saveDir = Mod::get()->getSaveDir();
                if (Loader::get()->isModLoaded("eclipse.eclipse-menu")) {
                    saveDir = Loader::get()->getLoadedMod("eclipse.eclipse-menu")->getSaveDir() / "replays";
                }

                if (!exists(saveDir)) {
                    create_directories(saveDir);
                }

                pick(PickMode::SaveFile, {
                    saveDir / fmt::format("{}.gdr", m_levelName),
                    {{
                        std::string("Macro File"),
                        std::unordered_set {std::string("gdr")}
                    }}
                }).listen([this, macro](auto path) {
                    if (path->isOk()) {
                        (void)writeBinary(path->unwrap(), macro);
                        removeFromParentAndCleanup(true);
                    }
                }, [](auto) {}, []() {});
            })
            .scale(0.8)
            .move(0, -40)
            .parent(getChildByID("menu"));
    }

    void keyBackClicked() override  {
        m_stop = true;
        CCLayer::keyBackClicked();
        removeFromParentAndCleanup(true);
    }

    bool init(std::string const& levelName, std::string const& lvlString) {
        CCLayerColor::initWithColor({0, 0, 0, 100});
        setCascadeOpacityEnabled(true);

        m_levelName = levelName;

        m_result = std::async(std::launch::async, [lvlString, this]() {
            try {
            return pathfind(lvlString, m_stop, [this](double progress) {
                if (m_progress < progress)
                    m_progress = progress;
            });
            } catch (std::exception& e) {
                log::error("{}", e.what());
                return std::vector<uint8_t>();
            }
        });

        setKeypadEnabled(true);
        Build(this)
            .initTouch()
            .schedule([this](float) {
                Build(this).intoChildRecurseID<CCLabelBMFont>("percent")
                    .string(fmt::format("{:.2f}%", m_progress).c_str());

                if (m_result.valid() && m_result.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    finalize(m_result.get());
                }
            })
            .intoNewChild(CCMenu::create())
                .id("menu")
                .intoNewChild(CCScale9Sprite::create("GJ_square04.png"))
                    .contentSize(250, 140)
                .intoNewSibling(CCLabelBMFont::create("Pathfinding...", "bigFont.fnt"))
                    .move(0, 50)
                    .scale(0.8)
                .intoNewSibling(CCLabelBMFont::create("0.00", "chatFont.fnt"))
                    .id("percent")
                    .move(0, 10)
                .intoNewSibling(ButtonSprite::create("Stop", "bigFont.fnt", "GJ_button_04.png"))
                    .intoMenuItem([this]() {
                        m_stop = true;
                        finalize(m_result.get());
                    })
                    .scale(0.8)
                    .id("cancel")
                    .move(0, -40)
                .intoNewSibling(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"))
                    .intoMenuItem([this]() {
                        m_stop = true;
                        this->removeFromParentAndCleanup(true);
                    })
                    .move(-125, 70)
                    .scale(0.8);
        ;

        return true;
    }

};

class $modify(EditLevelLayer) {
    bool init(GJGameLevel* p0) {
        EditLevelLayer::init(p0);
        if (m_level->isPlatformer()) return true;

        auto btn = Build<BasedButtonSprite>::create(
            CCSprite::create("pathfinder.png"_spr),
            BaseType::Circle,
            4,
            3
        ).scale(0.8);

        btn->setTopRelativeScale(1.4);

        auto clickable = btn.intoMenuItem([this]() {
            if (m_level->m_gameVersion > 17) {
                geode::createQuickPopup(
                    "Pathfinder",
                    "You are about to run the pathfinder on a level with <c_>UNSUPPORTED OBJECTS</c>.\n<cy>The pathfinder</c> <c_>WILL</c> <cy>produce a broken macro.</c>\nBy pressing \"Pathfind\", you are willing to expect <cy>a broken macro</c>.",
                    "Cancel", "Pathfind",
                    [this](FLAlertLayer* alert, bool pathfind) {
                        if (!pathfind) return;
                        Build<PathfinderNode>::create(m_level->m_levelName, ZipUtils::decompressString(m_level->m_levelString, true, 0)).parent(this).zOrder(100);
                    }
                );
            } else {
                auto lvlString = ZipUtils::decompressString(m_level->m_levelString, true, 0);
                Build<PathfinderNode>::create(m_level->m_levelName, lvlString).parent(this).zOrder(100);
            }
        }).id("pathfinder-button");

        if (geode::Mod::get()->getSettingValue<bool>("use-existing-menus")) {
            CCNode* levelActions = getChildByID("level-actions-menu");
            clickable.parent(levelActions);
            levelActions->updateLayout();
        } else {
          clickable.intoNewParent(CCMenu::create())
          .parent(this)
          .id("pathfinder-menu")
          .matchPos(getChildByIDRecursive("delete-button"))
          .move(-45, 0);
        }

        return true;
    }
};

class $modify(LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        LevelInfoLayer::init(level, challenge);
        if (m_level->isPlatformer()) return true;

        auto btn = Build<BasedButtonSprite>::create(
            CCSprite::create("pathfinder.png"_spr),
            BaseType::Circle,
            4,
            3
        ).scale(0.8);

        btn->setTopRelativeScale(1.4);

        auto clickable = btn.intoMenuItem([this]() {
            if (m_level->m_gameVersion > 17) {
                geode::createQuickPopup(
                    "Pathfinder",
                    "You are about to run the pathfinder on a level with <c_>UNSUPPORTED OBJECTS</c>.\n<cy>The pathfinder</c> <c_>WILL</c> <cy>produce a broken macro.</c>\nBy pressing \"Pathfind\", you are willing to expect <cy>a broken macro</c>.",
                    "Cancel", "Pathfind",
                    [this](FLAlertLayer* alert, bool pathfind) {
                        if (!pathfind) return;
                        Build<PathfinderNode>::create(m_level->m_levelName, ZipUtils::decompressString(m_level->m_levelString, true, 0)).parent(this).zOrder(100);
                    }
                );
            } else {
                auto lvlString = ZipUtils::decompressString(m_level->m_levelString, true, 0);
                Build<PathfinderNode>::create(m_level->m_levelName, lvlString).parent(this).zOrder(100);
            }
        }).id("pathfinder-button");

        if (geode::Mod::get()->getSettingValue<bool>("use-existing-menus")) {
            CCNode* leftSide = getChildByID("left-side-menu");
            clickable.parent(leftSide);
            leftSide->updateLayout();
        } else {
            clickable.parent(getChildByID("other-menu"))
            .matchPos(getChildByIDRecursive("list-button"))
            .move(0, 45);
        }

        return true;
    }
};
