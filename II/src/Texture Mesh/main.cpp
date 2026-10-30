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
#include "scenes.h"

using namespace std;

int init(int width, int height) {
	/* // Initialize SDL2. */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	/* Initialize SDL_image (JPG and PNG loaders). */
	int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
	if ((IMG_Init(img_flags) & img_flags) != img_flags) {
		cout << "IMG_Init Error: " << IMG_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	/* Create the window where we will draw. */
	window = SDL_CreateWindow("Texture Mesh", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
	if (window == nullptr) {
		cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		IMG_Quit();
		SDL_Quit();
		return 1;
	}

	/* We must call SDL_CreateRenderer in order for draw calls to affect this window. */
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr) {
		SDL_DestroyWindow(window);
		cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		IMG_Quit();
		SDL_Quit();
		return 1;
	}
	return 0;
}


void close() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}

int main(int argc, char* argv[])
{
	// Choose scene to render:
	// SceneData scene = create_scene_1_spotlight_only();
	// SceneData scene = create_scene_2_no_wall_all_lights();
	// SceneData scene = create_scene_3_three_spotlights();
	// SceneData scene = create_scene_4_extended_light_only();
	// SceneData scene = create_scene_5_all_lights();
	// SceneData scene = create_scene_6_original_cat();

	camera& cam = scene.cam;
	cam.initialize();

	// SDL e SDL_image devono essere inizializzati PRIMA di caricare la texture con IMG_Load
	if (init(cam.image_width, cam.image_height) == 1) {
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

	cam.parallel_render(scene.world, scene.lights, true);
	//cam.render(scene.world, scene.lights);

	auto end = std::chrono::steady_clock::now();
	double dif = std::chrono::duration<double>(end - start).count();
	cout << "\n" << "Rendering time: " << dif << " s\n";

	SDL_Event event;
	bool quit = false;

	/* Poll for events: si consumano TUTTI gli eventi in coda, poi si cede la CPU. */
	while (!quit) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {

			case SDL_QUIT:
				quit = true;
				break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
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
		SDL_Delay(10);
	}

	close();
	cleanup_scene(scene);
	return 0;
}
