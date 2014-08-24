#include "game.hpp"

#include "inugami/camera.hpp"

#include "meta.hpp"
#include "rect.hpp"
#include "components.hpp"

#include <tuple>
#include <string>
#include <functional>
#include <vector>

using namespace std;
using namespace Inugami;
using namespace Component;

// Draw Functions

    void Game::draw()
    {
        auto _ = profiler->scope("Game::draw()");

        beginFrame();

        Rect view = setupCamera();

        drawSprites(view);

        endFrame();
    }

    Rect Game::setupCamera()
    {
        auto _ = profiler->scope("Game::setupCamera()");

        Camera cam;
        cam.depthTest = true;

        Rect rv;

        {
            Rect view;
            view.left = numeric_limits<decltype(view.left)>::max();
            view.bottom = view.left;
            view.right = numeric_limits<decltype(view.right)>::lowest();
            view.top = view.right;

            for (auto& ent : active_world->query<Position, CamLook>())
            {
                auto& pos = get<1>(ent).data();
                auto& cam = get<2>(ent).data();

                view.left   = min(view.left,   pos.x + cam.aabb.left);
                view.bottom = min(view.bottom, pos.y + cam.aabb.bottom);
                view.right  = max(view.right,  pos.x + cam.aabb.right);
                view.top    = max(view.top,    pos.y + cam.aabb.top);
            }

            struct
            {
                double cx;
                double cy;
                double w;
                double h;
            } camloc =
            {     (view.left+view.right)/2.0
                , (view.bottom+view.top)/2.0
                , (view.right-view.left)
                , (view.top-view.bottom)
            };

            double rat = camloc.w/camloc.h;
            double trat = (min_view.width)/(min_view.height);

            if (rat < trat)
            {
                if (camloc.h < min_view.height)
                {
                    double s = min_view.height / camloc.h;
                    camloc.h = min_view.height;
                    camloc.w *= s;
                }

                camloc.w = trat*camloc.h;
            }
            else
            {
                if (camloc.w < min_view.width)
                {
                    double s = min_view.width / camloc.w;
                    camloc.w = min_view.width;
                    camloc.h *= s;
                }

                camloc.h = camloc.w/trat;
            }

            smoothcam.push(camloc.cx, camloc.cy, camloc.w, camloc.h);
            auto scam = smoothcam.get();
            scam.x = int(scam.x*pixel_scale)/double(pixel_scale);
            scam.y = int(scam.y*pixel_scale)/double(pixel_scale);

            double hw = scam.w/2.0;
            double hh = scam.h/2.0;
            rv.left = scam.x-hw;
            rv.right = scam.x+hw;
            rv.bottom = scam.y-hh;
            rv.top = scam.y+hh;

            cam.ortho(rv.left, rv.right, rv.bottom, rv.top, -10, 10);
        }

        applyCam(cam);

        return rv;
    }

    void Game::drawSprites(Rect view)
    {
        auto _ = profiler->scope("Game::drawSprites()");

        Transform mat;

        auto const& ents = active_world->query<Position, Sprite>();
        using Ent = decltype(&ents[0]);

        struct DrawItem
        {
            Ent ent;
            function<void()> draw;

            DrawItem(Ent e, function<void()> f)
                : ent(e)
                , draw(move(f))
            {}
        };

        vector<DrawItem> items;

        for (auto const& ent : ents)
        {
            auto& pos = get<1>(ent).data();
            auto& spr = get<2>(ent).data();

            auto const& sprdata = sprites.get(spr.name);

            Rect aabb;
            aabb.left   = pos.x-sprdata.width/2;
            aabb.right  = pos.x+sprdata.width/2;
            aabb.bottom = pos.y-sprdata.height/2;
            aabb.top    = pos.y+sprdata.height/2;

            if( aabb.left<view.right
            && aabb.right>view.left
            && aabb.bottom<view.top
            && aabb.top>view.bottom)
            {
                items.emplace_back(&ent, [&]
                {
                    auto _ = mat.scope_push();

                    auto const& anim = sprdata.anims.get(spr.anim);

                    mat.translate(int(pos.x+spr.offset.x), int(pos.y+spr.offset.y), pos.z);
                    if (spr.flipX) mat.scale(-1.0, 1.0);
                    modelMatrix(mat);

                    if (--spr.ticker <= 0)
                    {
                        ++spr.anim_frame;
                        if (spr.anim_frame >= anim.size())
                            spr.anim_frame = 0;
                        spr.ticker = anim[spr.anim_frame].duration;
                    }

                    auto const& frame = anim[spr.anim_frame];

                    sprdata.sheet.draw(frame.r, frame.c);
                });
            }
        }

        sort(begin(items), end(items), [](DrawItem const& a, DrawItem const& b)
        {
            auto& posa = get<1>(*a.ent).data();
            auto& posb = get<1>(*b.ent).data();
            return (tie(posa.z,posa.x,posa.y) < tie(posb.z,posb.x,posb.y));
        });

        for (auto const& item : items)
            item.draw();

        #if 0
        for (auto& ent : main_world.getEntities<Position, Solid>())
        {
            auto _ = mat.scope_push();

            auto& pos = *get<1>(ent);
            auto& solid = *get<2>(ent);

            mat.translate(pos.x, pos.y, pos.x+1);
            //mat.scale(solid.width/8.0, solid.height/8.0);
            modelMatrix(mat);

            auto const& sprdata = sprites.get("redx");
            auto const& anim = sprdata.anims.get("redx");
            auto const& frame = anim[0];

            sprdata.sheet.draw(frame.r, frame.c);
        }
        #endif
    }
