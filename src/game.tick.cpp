#include "game.hpp"

#include "inugami/camera.hpp"
#include "inugami/interface.hpp"

#include "meta.hpp"
#include "rect.hpp"
#include "components.hpp"
#include "level.hpp"

#include <tuple>
#include <vector>

using namespace std;
using namespace Inugami;
using namespace Component;

// Tick Functions

    void Game::tick()
    {
        auto _ = profiler->scope("Game::tick()");

        iface->poll();

        auto ESC = iface->key(Interface::ivkFunc(0));
        auto Z = iface->key(Interface::ivk('Z'));

        if (ESC || shouldClose())
        {
            running = false;
            return;
        }

        if (Z.pressed())
        {
            auto& pos = player.get<Position>().data();
            int r = (pos.y)/tileWidth;
            int c = (pos.x)/tileWidth;

            for (auto&& ent : active_world->query<Link,Position>())
            {
                Link& link = get<1>(ent).data();
                if (link.r == r && link.c == c)
                {
                    auto player_data = active_world->displaceEntity(player);
                    if (link.to_name == "main")
                    {
                        active_world = &main_world;
                    }
                    else
                    {
                        active_world = &digital_world;
                    }
                    player = active_world->emplaceEntity(move(player_data));
                    pos.y = link.to_r*tileWidth+tileWidth/2;
                    pos.x = link.to_c*tileWidth+tileWidth/2;

                    smoothcam.snapto(pos.x, pos.y, 0, 0);

                    break;
                }
            }
        }

        for (auto&& ent : active_world->query<AI>())
        {
            auto& ai = get<1>(ent).data();
            ai.clearSenses();
        }

        runPhysics();
        procAIs();
        slaughter();
    }

    void Game::procAIs()
    {
        auto _ = profiler->scope("Game::procAIs()");

        for (auto&& ent : active_world->query<AI>())
        {
            auto& e = get<0>(ent);
            auto& ai = get<1>(ent).data();
            ai.proc(*this, e);
        }
    }

    void Game::runPhysics()
    {
        using AIHitVec = vector<EntID> AI::Senses::*;

        auto _ = profiler->scope("Game::runPhysics()");

        auto const& ent_pos_vel_sol = active_world->query<Position,Velocity,Solid>();
        auto const& ent_vel_sol = active_world->query<Velocity,Solid>();
        auto const& ent_pos_sol = active_world->query<Position,Solid>();

        auto getRect = [](Position const& pos, Solid const& solid)
        {
            Rect rv;
            rv.left   = pos.x + solid.rect.left;
            rv.right  = pos.x + solid.rect.right;
            rv.bottom = pos.y + solid.rect.bottom;
            rv.top    = pos.y + solid.rect.top;
            return rv;
        };

#if 0
        struct BucketID
        {
            int x;
            int y;
        };

        unordered_map<Ginseng::Entity*, vector<BucketID>> bucketmap;
        map<BucketID, Ginseng::Entity*>

        auto setBuckets = [&](Ginseng::Entity& ent, Rect const& rect)
        {
            constexpr double bucketlength = (24.0*32.0);
            auto& buckets = bucketmap[&ent];

            buckets.clear();

            int bxb = int(rect.left/bucketlength);
            int bxe = int(rect.right/bucketlength);
            int byb = int(rect.bottom/bucketlength);
            int bye = int(rect.top/bucketlength);

            for (int x=bxb; x<=bxe; ++x)
                for (int y=byb; y<=bye; ++y)
                    buckets.emplace_back(x,y);
        };
#endif

        {
            auto _ = profiler->scope("Gravity");
            for (auto& ent : ent_vel_sol)
            {
                auto& vel = get<1>(ent).data();
                vel.vy -= 0.5;
            }
        }

        {
            auto _ = profiler->scope("Collision");

            for (auto& ent : ent_pos_vel_sol)
            {
                auto& eid   = get<0>(ent);
                auto& pos   = get<1>(ent).data();
                auto& vel   = get<2>(ent).data();
                auto& solid = get<3>(ent).data();

                auto ai = eid.get<AI>();

                auto linearCollide = [&](double Position::*d,
                                         double Velocity::*v,
                                         double Rect::*lower,
                                         double Rect::*upper,
                                         AIHitVec lhits,
                                         AIHitVec uhits)
                {
                    int hit = 0;
                    auto aabb = getRect(pos, solid);

                    for (auto& other : ent_pos_sol)
                    {
                        auto& eid2   = get<0>(other);
                        auto& pos2   = get<1>(other).data();
                        auto& solid2 = get<2>(other).data();

                        if (eid == eid2) continue;

                        auto aabb2 = getRect(pos2, solid2);

                        if (aabb.top > aabb2.bottom
                        &&  aabb.bottom < aabb2.top
                        &&  aabb.right > aabb2.left
                        &&  aabb.left < aabb2.right)
                        {
                            auto vel2info = eid2.get<Velocity>();

                            if (vel2info)
                            {
                                auto& vel2 = vel2info.data();
                                vel2.*v += vel.*v;
                            }

                            double overlap;

                            if (vel.*v > 0.0)
                                overlap = aabb2.*lower-aabb.*upper;
                            else
                                overlap = aabb2.*upper-aabb.*lower;

                            pos.*d += overlap;
                            aabb = getRect(pos, solid);
                            hit = (overlap>0?-1:1);

                            if (ai)
                            {
                                auto ai2 = eid2.get<AI>();

                                if (hit<0)
                                {
                                    if (ai2)
                                        (ai2.data().senses.*uhits).emplace_back(eid);
                                    (ai.data().senses.*lhits).emplace_back(eid2);
                                }
                                else
                                {
                                    if (ai2)
                                        (ai2.data().senses.*lhits).emplace_back(eid);
                                    (ai.data().senses.*uhits).emplace_back(eid2);
                                }
                            }
                        }
                    }

                    if (hit != 0)
                        vel.*v = 0.0;

                    return hit;
                };

                pos.x += vel.vx;
                int xhit = linearCollide(&Position::x,
                                         &Velocity::vx,
                                         &Rect::left,
                                         &Rect::right,
                                         &AI::Senses::hitsLeft,
                                         &AI::Senses::hitsRight);

                pos.y += vel.vy;
                int yhit = linearCollide(&Position::y,
                                         &Velocity::vy,
                                         &Rect::bottom,
                                         &Rect::top,
                                         &AI::Senses::hitsBottom,
                                         &AI::Senses::hitsTop);

                if (yhit != 0)
                    vel.vx *= 1.0-vel.friction;
            }
        }
    }

    void Game::slaughter()
    {
        auto _ = profiler->scope("Game::slaughter()");

        auto const& ents = active_world->query<KillMe>();

        for (auto& ent : ents)
            main_world.eraseEntity(get<0>(ent));
    }
