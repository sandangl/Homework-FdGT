#pragma once
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"
#include "color.h"

SDL_Window* window;
SDL_Renderer* renderer;

using namespace std;

inline void setColor(float r, float g, float b, float a = 1.0) {
	r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f : r);
	g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f : g);
	b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f : b);
	a = (a > 1.0f) ? 1.0f : ((a < 0.0f) ? 0.0f : a);
	SDL_SetRenderDrawColor(renderer, Uint8(r * 255.0), Uint8(g * 255.0), Uint8(b * 255.0), Uint8(a * 255.0));
}

inline void setPixel(int x, int y) {
	SDL_RenderDrawPoint(renderer, x, y);
}

inline void setPixel(int x, int y, float r, float g, float b, float a = 1.0) {
	setColor(r, g, b, a);
	setPixel(x, y);
}

inline void drawLine(int x1, int y1, int x2, int y2) {
	SDL_RenderDrawLine(renderer, x1, y1 - 1, x2, y2 - 1);
}

// Restituisce il pixel (x, y) della surface "impacchettato" secondo surface->format:
// per decodificare i canali usare SDL_GetRGB(value, surface->format, &r, &g, &b).
Uint32 getpixel(SDL_Surface* surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp) {
	case 1:
		return *p;
		break;

	case 2:
		return *(Uint16*)p;
		break;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;
		break;

	case 4:
		return *(Uint32*)p;
		break;

	default:
		return 0;       /* shouldn't happen, but avoids warnings */
	}
}

// Carica un'immagine (JPG, PNG, BMP, ...) e la converte in RGB24 (3 byte per pixel), cosi'
// getpixel + SDL_GetRGB funzionano allo stesso modo qualunque sia il formato del file.
SDL_Surface* loadTexture(const char* image_path, int& imageWidth, int& imageHeight)
{
	imageWidth = imageHeight = 0;

	SDL_Surface* loaded = IMG_Load(image_path);
	if (!loaded)
	{
		cout << "IMG_Load (" << image_path << "): " << IMG_GetError() << std::endl;
		return NULL;
	}

	SDL_Surface* image = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGB24, 0);
	SDL_FreeSurface(loaded);
	if (!image)
	{
		cout << "SDL_ConvertSurfaceFormat: " << SDL_GetError() << std::endl;
		return NULL;
	}

	imageWidth = image->w;
	imageHeight = image->h;
	return image;
}

bool saveScreenshotBMP(std::string filepath, SDL_Window* SDLWindow = window, SDL_Renderer* SDLRenderer = renderer) {
	SDL_Surface* saveSurface = NULL;
	SDL_Surface* infoSurface = NULL;
	infoSurface = SDL_GetWindowSurface(SDLWindow);
	if (infoSurface == NULL) {
		std::cerr << "Failed to create info surface from window in saveScreenshotBMP(string), SDL_GetError() - " << SDL_GetError() << "\n";
		return false;
	}

	unsigned char* pixels = new (std::nothrow) unsigned char[infoSurface->w * infoSurface->h * infoSurface->format->BytesPerPixel];
	if (pixels == 0) {
		std::cerr << "Unable to allocate memory for screenshot pixel data buffer!\n";
		return false;
	}

	if (SDL_RenderReadPixels(SDLRenderer, &infoSurface->clip_rect, infoSurface->format->format, pixels, infoSurface->w * infoSurface->format->BytesPerPixel) != 0) {
		std::cerr << "Failed to read pixel data from SDL_Renderer object. SDL_GetError() - " << SDL_GetError() << "\n";
		delete[] pixels;
		return false;
	}

	saveSurface = SDL_CreateRGBSurfaceFrom(pixels, infoSurface->w, infoSurface->h, infoSurface->format->BitsPerPixel, infoSurface->w * infoSurface->format->BytesPerPixel, infoSurface->format->Rmask, infoSurface->format->Gmask, infoSurface->format->Bmask, infoSurface->format->Amask);
	if (saveSurface == NULL) {
		std::cerr << "Couldn't create SDL_Surface from renderer pixel data. SDL_GetError() - " << SDL_GetError() << "\n";
		delete[] pixels;
		return false;
	}
	SDL_SaveBMP(saveSurface, filepath.c_str());
	SDL_FreeSurface(saveSurface);
	delete[] pixels;

	// NB: la surface restituita da SDL_GetWindowSurface appartiene alla finestra e NON va liberata.
	std::cout << "Image saved." << "\n";
	return true;
}
