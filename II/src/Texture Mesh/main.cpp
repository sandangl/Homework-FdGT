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
#include "short_scenes.h"

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

void load_short_script(vector<SceneFactory>& scenes, bool& record)
{
    bool valid = false;
    cout << "Vuoi avviare la registrazione? (y/n)";
    while (!valid)
    {
        cout << " > ";
        string ans;
        cin >> ans;
        if (ans == "y" || ans == "n" || ans == "Y" || ans == "N")
        {
            record = ans == "y" || ans == "Y";
            valid = true;
        } else valid = false;
        cin.ignore();
    }
    
    scenes.push_back([]{ return initial_scene(); });
    scenes.push_back([]{ return closeup_cactus_scene(); });
    scenes.push_back([]{ return rolling_tumbleweed_scene(); });
    scenes.push_back([]{ return turtle_scene_roadrunner(); });
    scenes.push_back([]{ return turtle_scene_coyote(); });
    scenes.push_back([]{ return running_coyote_scene(); });
    scenes.push_back([]{ return coyote_time_scene(); });
    scenes.push_back([]{ return final_falling_coyote_scene(); });
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
    double total_render_time = 0.0;
    
    bool quit = false;
    bool record = false;

    // choose_test_scene(scenes);
    load_short_script(scenes, record);

    int framecount = 0;
        
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
        
        if (scene.camera_positions.length() == 0)
        {
            scene.camera_positions.points.push_back(cam.lookfrom);
        }
        
        auto start = std::chrono::steady_clock::now();
        point3 base_lookat = cam.lookat;
        point3 base_lookfrom = cam.lookfrom;
        
        for (point3 camera_pos : scene.camera_positions.points)
        {
            framecount++;
            if (!scene.frame_transforms.empty())
            {
                auto& cur_frame_transforms = scene.frame_transforms.front();
                for (auto& transform : cur_frame_transforms)
                {
                    transform.apply_translation();
                    transform.apply_rotation();
                }
                scene.frame_transforms.pop_front();
            }
            else if (!scene.transforms.empty())
            {
                InstanceTransformations transform = scene.transforms.front();
                scene.transforms.pop_front();
                transform.apply_translation();
                transform.apply_rotation();
            }
            if (quit) break;
            cam.lookfrom = camera_pos;
            if (scene.move_also_lookat)
            {
                cam.lookat = base_lookat + (camera_pos - base_lookfrom);
            }
            cam.initialize();
            cam.parallel_render(scene.world, scene.lights);
            //cam.render(scene.world, scene.lights);

            SDL_Event event;
            SDL_PollEvent(&event);

            if (record)
            {
                stringstream ss;
                ss << framecount << ".bmp";
                saveScreenshotBMP(ss.str());
            }
            
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
                        if (!record)
                        {
                            saveScreenshotBMP("screenshot.bmp");
                            cout << "Screenshot saved!" << endl;
                        }
                        break;
                    }
                    break;
            }
        }

        cleanup_scene(scene);
        auto end = std::chrono::steady_clock::now();
        double dif = std::chrono::duration<double>(end - start).count();
        total_render_time += dif;
    }

    cout << "\n" << "Total rendering time: " << total_render_time << " s" << endl;

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
