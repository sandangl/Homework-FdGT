// Scelta dell'algoritmo di intersezione raggio-mesh:
//   USE_TINYBVH 1 -> mesh.h: BVH costruito da TinyBVH (veloce)
//   USE_TINYBVH 0 -> mesh_bruteforce.h: ciclo su tutti i triangoli (lento, ma e' la versione "didattica")
// Si puo' anche impostare dal progetto (Proprieta' -> C/C++ -> Preprocessore: USE_TINYBVH=0).
#ifndef USE_TINYBVH
#define USE_TINYBVH 1
#endif

#include <iostream>
#include <chrono>
#include <stdio.h>
#include "SDL.h"
#include "SDL_image.h"
#include "float.h"
#include "vec3.h"
#include "color.h"
#include "ray.h"
#include "geometry.h"
#include "cylinder.h"
#include "sphere.h"
#include "hittable.h"
#include "hittable_list.h"
#include "instance.h"
#if USE_TINYBVH
#include "mesh.h"
#else
#include "mesh_bruteforce.h"
#endif
#include "raster.h"
#include "object.h"
#include "camera.h"
#include "texture.h"
#include "ext_light.h"
#include "test_scenes.h"

using namespace std;

int init(int width, int height)
{
    /* Initialize SDL2 if needed. */
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return 1;
        }
    }

    /* Initialize SDL_image (JPG and PNG loaders). */
    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags)
    {
        cout << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    if (window != nullptr)
    {
        SDL_SetWindowSize(window, width, height);
        return 0;
    }

    /* Create the window where we will draw. */
    window = SDL_CreateWindow("Texture Mesh", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                              SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    /* We must call SDL_CreateRenderer in order for draw calls to affect this window. */
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        SDL_DestroyWindow(window);
        cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    return 0;
}


void close()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void choose_test_scene(vector<SceneFactory>& scenes)
{
    int choice;
    string typedChoice;

    cout << "Scegli una scena di test:" << endl;
    cout << "1: faretto" << endl;
    cout << "2: puntiforme, direzionale e faretto" << endl;
    cout << "3: tre faretti" << endl;
    cout << "4: luce estesa" << endl;
    cout << "5: tutte le luci" << endl;
    cout << "6: toon shading" << endl;
    cout << "7: puntiforme con gradiente" << endl;
    cout << "8: direzionale con cielo" << endl;
    cout << "9: primitive texture" << endl;
    cout << "10: sfere ferme" << endl;
    cout << "11: motion blur: sfera rimbalzante" << endl;
    cout << "12: Terra ferma" << endl;
    cout << "13: motion blur: Terra rotante" << endl;
    cout << "14: scena originale (gatto)" << endl << endl;
    cout << "0. Esci" << endl << endl;
    cout << "> ";

    bool chosen = false;
    while (!chosen)
    {
        chosen = true;
        cin >> typedChoice;

        try
        {
            choice = std::stoi(typedChoice);
        }
        catch (std::invalid_argument const& ex)
        {
            choice = -1;
        }

        switch (choice)
        {
        case 0:
            cout << "Ciao!";
            break;
        case 1:
            scenes.push_back([] { return create_scene_1_spotlight_only(); });
            break;
        case 2:
            scenes.push_back([] { return create_scene_2_no_wall_all_lights(); });
            break;
        case 3:
            scenes.push_back([] { return create_scene_3_three_spotlights(); });
            break;
        case 4:
            scenes.push_back([] { return create_scene_4_extended_light_only(); });
            break;
        case 5:
            scenes.push_back([] { return create_scene_5_all_lights(); });
            break;
        case 6:
            scenes.push_back([] { return create_toon_scene(); });
            break;
        case 7:
            scenes.push_back([] { return create_pointlight_and_gradient_scene(); });
            break;
        case 8:
            scenes.push_back([] { return create_sky_scene(); });
            break;
        case 9:
            scenes.push_back([] { return create_solid_with_textures_scene(); });
            break;
        case 10:
            scenes.push_back([] { return create_motion_blur_bouncing_scene(false); });
            break;
        case 11:
            scenes.push_back([] { return create_motion_blur_bouncing_scene(true); });
            break;
        case 12:
            scenes.push_back([] { return create_motion_blur_rotating_scene(false); });
            break;
        case 13:
            scenes.push_back([] { return create_motion_blur_rotating_scene(true); });
            break;
        case 14:
            scenes.push_back([] { return create_scene_6_original_cat(); });
            break;
        default:
            cout << "\nNon valido.";
            chosen = false;
        }
    }
}

void load_short_script(vector<SceneFactory>& scenes)
{
    
}

int main(int argc, char* argv[])
{
    // Initialize SDL and SDL_image early so factories can load textures safely
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags)
    {
        cout << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    vector<SceneFactory> scenes;

    choose_test_scene(scenes);
    //load_short_script(scenes);

    bool quit = false;

    if (scenes.size() == 0) quit = true;

    for (auto& make_scene : scenes)
    {
        if (quit)
        {
            break;
        }

        SceneData scene = make_scene();
        camera& cam = scene.cam;
        cam.initialize();

        if (init(cam.image_width, cam.image_height) == 1)
        {
            cout << "App Error! " << std::endl;
            cleanup_scene(scene);
            return 1;
        }

        cout << "Image Resolution: " << cam.image_width << "x" << cam.image_height << "\n255\n";

#if USE_TINYBVH
        cout << "Ray-mesh intersection: TinyBVH" << endl;
#else
        cout << "Ray-mesh intersection: brute force (all triangles)" << endl;
#endif

        auto start = std::chrono::steady_clock::now();

        if (scene.camera_positions.length() == 0)
        {
            scene.camera_positions.points.push_back(cam.lookfrom);
        }
        for (point3 camera_pos : scene.camera_positions.points)
        {
            if (quit) break;
            cam.lookfrom = camera_pos;
            cam.parallel_render(scene.world, scene.lights);
            //cam.render(scene.world, scene.lights);

            auto end = std::chrono::steady_clock::now();
            double dif = std::chrono::duration<double>(end - start).count();
            cout << "\n" << "Rendering time: " << dif << " s" << endl;

            SDL_Event event;
            SDL_PollEvent(&event);

            switch (event.type)
            {
                case SDL_QUIT:
                    quit = true;
                    break;

                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                        quit = true;
                        break;

                    case SDLK_s:
                        saveScreenshotBMP("screenshot.bmp");
                        cout << "Screenshot saved!" << endl;
                        break;
                    }
                    break;
            }
        }

        cleanup_scene(scene);
    }

    cout << "\n\nFinished rendering all scenes. Press any key to quit...\n" << endl;
    
    SDL_Event event;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_KEYDOWN:
                quit = true;
                break;
            }
        }
        SDL_Delay(10);
    }

    close();

    return 0;
}
