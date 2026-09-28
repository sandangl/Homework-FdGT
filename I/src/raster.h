#pragma once
#include "SDL.h"

SDL_Window* window;
SDL_Renderer* renderer;

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

bool saveScreenshotBMP(std::string filepath, SDL_Window* SDLWindow = window, SDL_Renderer* SDLRenderer = renderer) {
	SDL_Surface* saveSurface = NULL;
	SDL_Surface* infoSurface = NULL;
	infoSurface = SDL_GetWindowSurface(SDLWindow);
	if (infoSurface == NULL) {
		std::cerr << "Failed to create info surface from window in saveScreenshotBMP(string), SDL_GetError() - " << SDL_GetError() << "\n";
	}
	else {
		unsigned char* pixels = new (std::nothrow) unsigned char[infoSurface->w * infoSurface->h * infoSurface->format->BytesPerPixel];
		if (pixels == 0) {
			std::cerr << "Unable to allocate memory for screenshot pixel data buffer!\n";
			return false;
		}
		else {
			if (SDL_RenderReadPixels(SDLRenderer, &infoSurface->clip_rect, infoSurface->format->format, pixels, infoSurface->w * infoSurface->format->BytesPerPixel) != 0) {
				std::cerr << "Failed to read pixel data from SDL_Renderer object. SDL_GetError() - " << SDL_GetError() << "\n";
				pixels = NULL;
				return false;
			}
			else {
				saveSurface = SDL_CreateRGBSurfaceFrom(pixels, infoSurface->w, infoSurface->h, infoSurface->format->BitsPerPixel, infoSurface->w * infoSurface->format->BytesPerPixel, infoSurface->format->Rmask, infoSurface->format->Gmask, infoSurface->format->Bmask, infoSurface->format->Amask);
				if (saveSurface == NULL) {
					std::cerr << "Couldn't create SDL_Surface from renderer pixel data. SDL_GetError() - " << SDL_GetError() << "\n";
					return false;
				}
				SDL_SaveBMP(saveSurface, filepath.c_str());
				SDL_FreeSurface(saveSurface);
				saveSurface = NULL;
			}
			delete[] pixels;
		}
		SDL_FreeSurface(infoSurface);
		infoSurface = NULL;
	}
	std::cout << "Image saved." << "\n";
	return true;
}