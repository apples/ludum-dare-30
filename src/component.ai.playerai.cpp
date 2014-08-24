#include "component.ai.playerai.hpp"

#include "components.hpp"

#include "component.position.hpp"
#include "component.velocity.hpp"

#include <tuple>
using namespace std;

namespace Component {

void PlayerAI::operator()(Game& game, EntID ent, AI const& ai)
{
    auto comps = ent.getComs<Position,Velocity,Sprite>();
    auto& pos = get<0>(comps).data();
    auto& vel = get<1>(comps).data();
    auto& sprite = get<2>(comps).data();

    if (inputs[LEFT]() && vel.vx>-5.0)
    {
        vel.vx -= min(vel.vx+5.0,5.0);
        sprite.flipX = true;
    }

    if (inputs[RIGHT]() && vel.vx<5.0)
    {
        vel.vx += min(5.0-vel.vx,5.0);
        sprite.flipX = false;
    }

    if (inputs[UP]() && !ai.senses.hitsBottom.empty()) vel.vy += 11;

    if (ai.senses.hitsBottom.empty() && !inputs[LEFT]() && !inputs[RIGHT]())
    {
        vel.vx *= 0.5;
    }

    if (vel.vx != 0.0 && sprite.anim != "walk")
    {
        sprite.reset_anim("walk");
    }
    else if (vel.vx == 0.0 && sprite.anim != "idle")
    {
        sprite.reset_anim("idle");
    }

    if (vel.vy > 0.0 && sprite.anim != "jump")
    {
        sprite.reset_anim("jump");
    }
}

void PlayerAI::setInput(Input i, std::function<bool()> func)
{
    inputs[i] = std::move(func);
}

} // namespace Component
