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
	// Camera: va inizializzata prima di SDL perche' fornisce le dimensioni della finestra
	camera cam;
	cam.lookfrom = point3(0.0f, 50.0f, 30.0f);
	cam.lookat = point3(0.0f, -30.0f, -120.0f);

	cam.aspect_ratio = 16.0f / 9.0f;
	cam.image_width = 700;
	cam.samples_per_pixel = 1;
	cam.vfov = 20;

	cam.initialize();

	// Luce e materiale della scena
	color lightgray = color(0.75f, 0.75f, 0.75f);
	color faintred = color(1.0f, 0.55f, 0.55f);
	color faintblue = color(0.55f, 0.55f, 1.0f);
	color black = color(0.0f, 0.0f, 0.0f);
	point3 light_position(6.0f, 6.0f, 0.0f);
	point_light* worldlight = new point_light(light_position, black, lightgray, black);
	
	// Spotlight allineata con camera lookfrom, ma rialzata, puntata sul bunny
	point3 spotlight_position(30.0f, 60.0f, -60.0f);
	vec3 spotlight_direction = point3(0.0f, -40.0f, -120.0f) - spotlight_position;
	spotlight* spot = new spotlight(spotlight_position, spotlight_direction, 20.0f, 0.3f, color(0.05f, 0.05f, 0.05f), color(0.9f, 0.9f, 0.9f), color(0.5f, 0.5f, 0.5f));

	// SDL e SDL_image devono essere inizializzati PRIMA di caricare la texture con IMG_Load
	if (init(cam.image_width, cam.image_height) == 1) {
		cout << "App Error! " << std::endl;
		return 1;
	}

	// World
	hittable_list world;

	// Texture Mapping: mesh del gatto con la sua texture
	// mesh* model3d = new mesh("../models/cat.obj", "../models/");
	// if (!model3d->loaded) {
	// 	cout << "Impossibile caricare il modello ../models/cat.obj" << std::endl;
	// 	close();
	// 	return 1;
	// }
	// texture* cat_texture = new image_texture("../models/texturecat.jpg");
	// material* m = new material();
	// m->tex = cat_texture;
	// m->ka = lightgray;
	// m->ks = lightgray;
	
	mesh* model3d = new mesh("../models/bunny.obj", "../models/");
	if (!model3d->loaded) {
		cout << "Impossibile caricare il modello ../models/bunny2.obj" << std::endl;
		close();
		return 1;
	}
	material* m = new material();
	m->ka = lightgray;
	m->kd = lightgray;
	m->ks = lightgray;

	auto instance_ptr = make_shared<instance>(model3d, m);
	instance_ptr->scale(10.0f, 10.0f, 10.0f);
	instance_ptr->rotate_y(8.0f);
	instance_ptr->rotate_z(0.0f);
	instance_ptr->translate(0.0f, -40.0f, -120.0f);
	world.add(instance_ptr);
	
	// Sfere
	material* red_sphere_mat = new material();
	red_sphere_mat->ka = faintred;
	red_sphere_mat->kd = faintred;
	red_sphere_mat->ks = faintred;
	
	material* blue_sphere_mat = new material();
	blue_sphere_mat->ka = faintblue;
	blue_sphere_mat->kd = faintblue;
	blue_sphere_mat->ks = faintblue;
	
	auto red_sphere = make_shared<sphere>(point3(-27.0f, -30.0f,-118.0f), 10.0f, red_sphere_mat);
	auto blue_sphere = make_shared<sphere>(point3(23.0f, -32.0f, -115.0f), 8.0f, blue_sphere_mat);
	world.add(red_sphere);
	world.add(blue_sphere);

	// Stanza: pavimento sotto il bunny (y = -40) e parete a 90° dietro il bunny (z = -128)
	texture* wallfloor_texture = new image_texture("../models/texturecheckerboard.png");
	material* floor_mat = new material();
	floor_mat->tex = wallfloor_texture;
	floor_mat->ka = color(0.2f, 0.2f, 0.2f);
	floor_mat->kd = color(0.5f, 0.5f, 0.5f);
	floor_mat->ks = color(0.1f, 0.1f, 0.1f);
	floor_mat->alpha = 10.0f;

	material* wall_mat = new material();
	wall_mat->tex = wallfloor_texture;
	wall_mat->ka = color(0.2f, 0.2f, 0.22f);
	wall_mat->kd = color(0.6f, 0.6f, 0.65f);
	wall_mat->ks = color(0.1f, 0.1f, 0.1f);
	wall_mat->alpha = 10.0f;

	auto floor_plane = make_shared<plane>(point3(0.0f, -40.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), floor_mat);
	auto back_wall = make_shared<plane>(point3(0.0f, 0.0f, -128.0f), vec3(0.0f, 0.0f, 1.0f), wall_mat);
	world.add(floor_plane);
	world.add(back_wall);
	
	

	cout << "Image Resolution: " << cam.image_width << "x" << cam.image_height << "\n255\n";

#if USE_TINYBVH
	cout << "Ray-mesh intersection: TinyBVH" << endl;
#else
	cout << "Ray-mesh intersection: brute force (all triangles)" << endl;
#endif

	auto start = std::chrono::steady_clock::now();

	cam.parallel_render(world, *spot);
	//cam.render(world, *spot);

	auto end = std::chrono::steady_clock::now();
	double dif = std::chrono::duration<double>(end - start).count();
	cout << "\n" << "Rendering time: " << dif << " s\n";

	SDL_Event event;
	bool quit = false;

	/* Poll for events: si consumano TUTTI gli eventi in coda, poi si cede la CPU.
	   (Il vecchio "while (SDL_PollEvent(&event) || !quit)" rielaborava l'ultimo evento
	   all'infinito quando la coda era vuota, ad es. salvando screenshot in continuazione.) */
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
					// cases for other keypresses

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

	//delete cat_texture;
	delete m;
	delete floor_mat;
	delete wall_mat;
	delete model3d;
	delete worldlight;
	delete spot;
	return 0;
}
