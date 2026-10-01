#include "SDL.h" 
#include <iostream>
#include "common.h"

#include "color.h"
#include "hittable.h"
#include "hittable_list.h"
#include "geometry.h"
#include "raster.h"
#include "camera.h"

using namespace std;

int init(int width, int height) {
	/* // Initialize SDL2. */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	/* Create the window where we will draw. */
	window = SDL_CreateWindow("Multiple Objects", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
	if (window == nullptr) {
		cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	/* We must call SDL_CreateRenderer in order for draw calls to affect this window. */
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr) {
		SDL_DestroyWindow(window);
		cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	return 0;
}

void close() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}



int main(int argc, char* argv[])
{
	// World
	hittable_list world;

	world.add(make_shared<sphere>(point3(0.0f, -100.5f, -1.0f), 100.0f));
	world.add(make_shared<sphere>(point3(1.0f, 0.0f, -0.0f), 0.3f));
	world.add(make_shared<sphere>(point3(2.5f, 1.0f, -0.5f), 0.8f));
	world.add(make_shared<sphere>(point3(1.8f, 0.1f, -1.0f), 0.3f));
	world.add(make_shared<sphere>(point3(-2.5f, 0.2f, -3.0f), 0.4f));
	world.add(make_shared<sphere>(point3(1.2f, 1.2f, -2.5f), 0.3f));
	world.add(make_shared<sphere>(point3(1.0f, 2.4f, -3.5f), 0.1f));
	world.add(make_shared<sphere>(point3(0.3f, 2.4f, 3.5f), 0.5f));
	world.add(make_shared<sphere>(point3(-2.5f, 0.2f, -3.0f), 0.7f));
	world.add(make_shared<sphere>(point3(-3.5f, -0.5f, -3.0f), 0.5f));


	camera cam;
	cam.aspect_ratio = 16.0f / 9.0f;
	cam.image_width = 1280;
	cam.samples_per_pixel = 8;
	
	cam.vfov = 90;
	cam.lookfrom = point3(2, 3, -4);
	cam.lookat = point3(0, 0, -1);
		
	cam.initialize();
	
	if (init(cam.image_width, cam.image_height) == 1) {
		cout << "App Error! " << std::endl;
		return 1;
	}

	// Interocular distance
	//
	// Typical values:
	//    - interocular distance: 6.5 cm (approx. 246px @ 1280x720)
	//	  - 1% of viewport width
	//	  - 5% of viewport width
	//
	// By multiplying e by -1, the anaglyph gets inverted with an inward 3D effect.

	float e = cam.image_width * 0.05;

	//cam.render(world, true, e);
	cam.parallel_render(world, true, e);

	SDL_RenderPresent(renderer);

	SDL_Event event;
	bool quit = false;

	while (SDL_PollEvent(&event) || (!quit)) {

		switch (event.type) {

		case SDL_QUIT:
			quit = true;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
					// cases for other keypresses

				case SDLK_s:
					saveScreenshotBMP("screenshot.bmp");
					break;
			}
		}
	}

	close();
	return 0;
}

