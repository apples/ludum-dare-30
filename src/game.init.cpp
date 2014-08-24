#include "game.hpp"

#include "inugami/camera.hpp"
#include "inugami/interface.hpp"

#include "meta.hpp"
#include "level.hpp"
#include "components.hpp"

#include <tuple>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

using namespace std;
using namespace Inugami;
using namespace Component;

// Constructor

    Game::Game(RenderParams params)
        : Core(params)
        , rng(nd_rand())
    {
        auto _ = profiler->scope("Game::<constructor>()");

        params = getParams();

    // Configuration

        addCallback([&]{ tick(); draw(); }, 60.0);
        setWindowTitle("Ludum Dare 30", true);

        min_view.width = (params.width/pixel_scale);
        min_view.height = (params.height/pixel_scale);

        smoothcam = SmoothCamera(10);

    // Resources

        loadTextures();
        loadSprites();

    // Entities

        active_world = &main_world;

        // Player

            auto make_player = [&](ECDatabase& db)
            {
                auto ent = db.makeEntity();

                auto& sprite = db.makeComponent(ent, Sprite{}).data();
                sprite.name = "player";
                sprite.anim = "idle";

                auto& pos = db.makeComponent(ent, Position{}).data();
                pos.x = 64;
                pos.y = 64;

                auto& vel = db.makeComponent(ent, Velocity{}).data();

                auto& solid = db.makeComponent(ent, Solid{}).data();
                solid.rect.left = -8;
                solid.rect.right = solid.rect.left + 16;
                solid.rect.bottom = -16;
                solid.rect.top = solid.rect.bottom + 31;

                PlayerAI ai;
                ai.setInput(PlayerAI::LEFT,  iface->key(Interface::ivkArrow('L')));
                ai.setInput(PlayerAI::RIGHT, iface->key(Interface::ivkArrow('R')));
                ai.setInput(PlayerAI::DOWN,  iface->key(Interface::ivkArrow('D')));
                ai.setInput(PlayerAI::UP,    iface->key(Interface::ivkArrow('U')));
                auto& aic = db.makeComponent(ent, AI{move(ai)}).data();

                auto& cam = db.makeComponent(ent, CamLook{}).data();
                cam.aabb = solid.rect;

                return ent;
            };

            player = make_player(main_world);

    // Load Level

        Level lvl;
        lvl.loadFile("data/test-level.lvl");

        Stage& main_stage = lvl.stages.at("main");
        Stage& digital_stage = lvl.stages.at("digital");

        auto load_world = [&](Stage& stg, ECDatabase& db)
        {
            std::string tileset_name = "tiles-"+stg.name;

            logger->log("Loading tiles for ", stg.name);

            for (int r=0; r<stg.getHeight(); ++r)
            {
                for (int c=0; c<stg.getWidth(); ++c)
                {
                    auto tile = db.makeEntity();

                    auto pos = db.makeComponent(tile, Position{});
                    pos.data().y = r*tileWidth+tileWidth/2;
                    pos.data().x = c*tileWidth+tileWidth/2;

                    auto sprite = db.makeComponent(tile, Sprite{});
                    sprite.data().name = tileset_name;

                    auto t = db.makeComponent(tile, Tile{});
                    t.data().r = r;
                    t.data().c = c;

                    switch (stg[r][c])
                    {
                        case 1:
                        {
                            sprite.data().anim = "wall";
                            auto& solid = db.makeComponent(tile, Solid{}).data();
                            solid.rect.left = -tileWidth/2;
                            solid.rect.right = tileWidth/2;
                            solid.rect.bottom = -tileWidth/2;
                            solid.rect.top = tileWidth/2;
                        } break;

                        case 2:
                        {
                            sprite.data().anim = "door-closed";
                            auto& solid = db.makeComponent(tile, Solid{}).data();
                            solid.rect.left = -tileWidth/2;
                            solid.rect.right = tileWidth/2;
                            solid.rect.bottom = -tileWidth/2;
                            solid.rect.top = tileWidth/2;

                            auto& dor = db.makeComponent(tile, Door{}).data();
                            dor.open = false;
                        } break;

                        default:
                        {
                            sprite.data().anim = "background";
                            pos.data().z = -1;
                        }
                    }
                }
            }

            logger->log("Loading links for ", stg.name);

            for (Link& l : stg.links)
            {
                auto ent = db.makeEntity();

                auto& pos = db.makeComponent(ent, Position{}).data();
                pos.y = l.r*tileWidth+tileWidth/2;
                pos.x = l.c*tileWidth+tileWidth/2;

                auto& sprite = db.makeComponent(ent, Sprite{}).data();
                sprite.name = tileset_name;
                sprite.anim = "terminal";

                db.makeComponent(ent, l);
            }

            logger->log("Loading switches for ", stg.name);

            for (Switch& sw : stg.switches)
            {
                auto ent = db.makeEntity();

                auto& pos = db.makeComponent(ent, Position{}).data();
                pos.y = sw.r*tileWidth+tileWidth/2;
                pos.x = sw.c*tileWidth+tileWidth/2;

                auto& sprite = db.makeComponent(ent, Sprite{}).data();
                sprite.name = tileset_name;
                sprite.anim = "switch";

                db.makeComponent(ent, sw);
            }

            logger->log("Done loading ", stg.name);
        };

        load_world(main_stage, main_world);
        load_world(digital_stage, digital_world);
    }

// Resource and Configuration Functions

    void Game::loadTextures()
    {
        using namespace YAML;

        auto conf = YAML::LoadFile("data/textures.yaml");

        for (auto const& p : conf)
        {
            auto file_node   = p.second["file"];
            auto smooth_node = p.second["smooth"];
            auto clamp_node  = p.second["clamp"];

            auto name = p.first.as<string>();
            auto file = file_node.as<string>();
            auto smooth = (smooth_node? smooth_node.as<bool>() : false);
            auto clamp  = ( clamp_node?  clamp_node.as<bool>() : false);

            textures.create(name, Image::fromPNG("data/"+file), smooth, clamp);
        }
    }

    void Game::loadSprites()
    {
        using namespace YAML;

        auto conf = LoadFile("data/sprites.yaml");

        for (auto const& spr : conf)
        {
            auto name = spr.first.as<string>();
            SpriteData data;

            auto texname = spr.second["texture"].as<string>();
            auto width = spr.second["width"].as<int>();
            auto height = spr.second["height"].as<int>();
            auto const& anims = spr.second["anims"];

            data.sheet = Spritesheet(textures.get(texname), width, height);
            data.width = width;
            data.height = height;

            for (auto const& anim : anims)
            {
                auto anim_name = anim.first.as<string>();
                auto& frame_vec = data.anims.create(anim_name);

                for (auto const& frame : anim.second)
                {
                    auto r_node = frame["r"];
                    auto c_node = frame["c"];
                    auto dur_node = frame["dur"];

                    auto r = r_node.as<int>();
                    auto c = c_node.as<int>();
                    auto dur = dur_node.as<int>();

                    frame_vec.emplace_back(r, c, dur);
                }
            }

            sprites.create(name, move(data));
        }
    }
