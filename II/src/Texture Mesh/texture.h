#pragma once
#include <cmath>
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"
#include "vec3.h"
#include "raster.h"

using namespace std;

class texture {
public:
	virtual ~texture() = default;
	virtual color value(float u, float v, const point3& p) const = 0;
};

class constant_texture : public texture {
public:
	constant_texture() { }

	constant_texture(color col) : c(col) { }

	virtual color value(float u, float v, const point3& p) const {
		return c;
	}

	// NB: il membro non puo' chiamarsi "color" come il tipo (g++: "changes meaning of 'color'").
	color c;
};

class checker_texture : public texture {
public:
	checker_texture() { }

	checker_texture(texture* t0, texture* t1) : even(t0), odd(t1) { }

	virtual color value(float u, float v, const point3& p) const {
		float sines = sin(2 * p[0]) * sin(2 * p[1]) * sin(2 * p[2]);
		if (sines < 0)
			return odd->value(u, v, p);
		else
			return even->value(u, v, p);
	}

	texture* even;
	texture* odd;
};

class image_texture : public texture {
public:
	image_texture(const char* filename) {
		surface = loadTexture(filename, imageWidth, imageHeight);
		if (surface == NULL)
			cerr << "image_texture: impossibile caricare " << filename << endl;
	}

	~image_texture() {
		if (surface) SDL_FreeSurface(surface);
	}

	virtual color value(float u, float v, const point3& p) const;

	SDL_Surface* surface = NULL;
	int imageWidth = 0, imageHeight = 0;
};

color image_texture::value(float u, float v, const point3& p) const {
	// Senza immagine si restituisce magenta, cosi' l'errore e' visibile nel rendering.
	if (surface == NULL)
		return color(1.0f, 0.0f, 1.0f);

	// (u, v) in [0, 1]: u cresce verso destra, v verso l'alto (nel file l'immagine
	// e' memorizzata dall'alto verso il basso, quindi la riga e' (1 - v) * altezza).
	int i = int(u * float(imageWidth));
	int j = int((1.0f - v) * float(imageHeight) - 0.001f);
	if (i < 0) i = 0;
	if (j < 0) j = 0;
	if (i > imageWidth - 1) i = imageWidth - 1;
	if (j > imageHeight - 1) j = imageHeight - 1;

	// SDL_GetRGB estrae i canali secondo il formato della surface: funziona con qualunque
	// ordine dei byte (RGB24, BGR24, RGBA32, ...), senza dover "indovinare" gli shift.
	Uint32 value = getpixel(surface, i, j);
	Uint8 r, g, b;
	SDL_GetRGB(value, surface->format, &r, &g, &b);

	return color(r / 255.0f, g / 255.0f, b / 255.0f);
}
