#ifndef GAME_HPP
#define GAME_HPP

#include "inugami/core.hpp"
#include "inugami/texture.hpp"
#include "inugami/spritesheet.hpp"

#include "puddle/puddle.hpp"

#include "resourcepool.hpp"
#include "spritedata.hpp"
#include "rect.hpp"
#include "smoothcamera.hpp"
#include "types.hpp"

#include "SFML/Audio.hpp"

#include <memory>
#include <random>
#include <utility>

class Game
	: public Inugami::Core
{
    // Configuration

        int tileWidth = 32;

        struct
        {
            double width;
            double height;
        } min_view;

		int pixel_scale = 2;

        SmoothCamera smoothcam;

    // Resources

        ResourcePool<Inugami::Texture> textures;
        ResourcePool<SpriteData> sprites;

    // Support

        std::mt19937 rng;

public:

    // Entities

        ECDatabase main_world;
        ECDatabase digital_world;

		ECDatabase* active_world;
		EntID player;

	// Music

		sf::Music main_music;
		sf::Music digital_music;

    // Initialization

        Game(RenderParams params);

        void loadTextures();
        void loadSprites();
        void loadMusic();

    // Tick Functions

        void tick();

        void procAIs();
        void runPhysics();
        void slaughter();

    // Draw Functions

        void draw();

        Rect setupCamera();
        void drawSprites(Rect view);
};

#endif // GAME_HPP
