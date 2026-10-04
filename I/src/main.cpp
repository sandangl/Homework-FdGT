#include "SDL.h" 
#include <iostream>

#include "common.h"
#include "color.h"
#include "hittable.h"
#include "hittable_list.h"
#include "point3_list.h"
#include "geometry.h"
#include "raster.h"
#include "camera.h"
#include "spline.h"

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

	const float e = cam.image_width * 0.05f;

	// Interpolation points
	//
	// Camera path gets calculated using a Catmull-Rom spline.
	// Given 5 points, we can regulate t parameter between 0 and 1 to regulate how dense
	// the resulting interpolated path is, from coarser (epsilon) to finest (1);

	point3_list camPositions;
	camPositions.add(point3(2, 3, -4));
	camPositions.add(point3(4, 4, -2));
	camPositions.add(point3(4, 2, 0));
	camPositions.add(point3(3, 3, 0));
	camPositions.add(point3(3, 1, 2));
	camPositions.add(point3(2, 5, 4));

	float dt = (camPositions.length() * 1.0f)/250.0f;

	path camPath = catmull_rom_interpolate(camPositions, dt);

	SDL_Event event;
	int frame_index = 0;
	bool rendered = false;
	bool is_rendering = false;
	bool toggle_animation = false;
	bool toggle_anaglyph = true;
	bool enable_recording = true;
	float current_e = e;
	bool quit = false;

	while (SDL_PollEvent(&event) || (!quit)) {

		if(!rendered) {
			if (toggle_animation) {
				if (frame_index > camPath.points.size() - 1) {
					frame_index = 0;
					if (enable_recording) {
						cout << "Recording complete." << endl;
						enable_recording = false;
					}
				}
				cam.lookfrom = camPath.points.at(frame_index);
				cam.updateCameraPosition();
				cam.parallel_render(world, toggle_anaglyph, e);
				SDL_RenderPresent(renderer);
				if (enable_recording) {
					saveCurrentFrame(frame_index);
				}
				frame_index++;
			}
			else {
				cam.parallel_render(world, toggle_anaglyph, current_e);
				SDL_RenderPresent(renderer);
				rendered = true;
			}
		}

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

					case SDLK_a:
						current_e = toggle_anaglyph ? 0 : e;
						toggle_anaglyph = !toggle_anaglyph;
						rendered = false;
						break;

					case SDLK_r:
						enable_recording = !enable_recording;
						cout << (enable_recording ? "Auto frame saving enabled." : "Auto frame saving disabled.") << endl;
						break;

					case SDLK_SPACE:
						frame_index = 0;
						toggle_animation = !toggle_animation;
						is_rendering = !is_rendering;
						rendered = false;
						break;

					case SDLK_h:
						if (!is_rendering) {
							cam.lookfrom += vec3(0.0f, 0.0f, 0.5f);
							cam.updateCameraPosition();
							rendered = false;
						}
						break;

					case SDLK_j:
						if (!is_rendering) {
							cam.lookfrom += vec3(0.0f, 0.0f, -0.5f);
							cam.updateCameraPosition();
							rendered = false;
						}
						break;

					case SDLK_k:
						if (!is_rendering) {
							cam.lookfrom += vec3(0.5f, 0.0f, 0.0f);
							cam.updateCameraPosition();
							rendered = false;
						}
						break;

					case SDLK_l:
						if (!is_rendering) {
							cam.lookfrom += vec3(-0.5f, 0.0f, 0.0f);
							cam.updateCameraPosition();
							rendered = false;
						}
						break;

					case SDLK_COMMA:
						if (!is_rendering) {
							cam.lookfrom += vec3(0.0f, 0.5f, 0.0f);
							cam.updateCameraPosition();
							rendered = false;
						}
						break;

					case SDLK_PERIOD:
						if (!is_rendering) {
							cam.lookfrom += vec3(0.0f, -0.5f, 0.0f);
							cam.updateCameraPosition();
							rendered = false;
						}
						break;

				}
			}
	}

	close();
	return 0;
}

