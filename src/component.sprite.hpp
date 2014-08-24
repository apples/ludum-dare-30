#ifndef COMPONENT_SPRITE_HPP
#define COMPONENT_SPRITE_HPP

#include <string>

namespace Component {

struct Sprite
{
    std::string name;
    std::string anim;
    unsigned anim_frame = -1;
    int ticker = 0;

    struct
    {
        double x = 0;
        double y = 0;
    }
    offset;

    bool flipX = false;

    void reset_anim(std::string const& newanim)
    {
        anim = newanim;
        anim_frame = -1;
        ticker = 0;
    }

    void reset(std::string const& newname, std::string const& newanim)
    {
        name = newname;
        anim = newanim;
        anim_frame = -1;
        ticker = 0;
    }
};

} // namespace Component

#endif // COMPONENT_SPRITE_HPP
