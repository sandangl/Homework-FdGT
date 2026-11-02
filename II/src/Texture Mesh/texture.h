#pragma once
#include <cmath>
#include <iostream>
#include <algorithm>
#include "SDL.h"
#include "SDL_image.h"
#include "vec3.h"
#include "common.h"
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
	
	float repeat_u = 1.0f;
	float repeat_v = 1.0f;
	
	image_texture() = default;

	image_texture(const char* filename, float rep_u = 1.0f, float rep_v = 1.0f) {
		repeat_u = rep_u;
		repeat_v = rep_v;
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
	if (surface == NULL)
		return color(1.0f, 0.0f, 1.0f);

	// Repeat factor
	u = u * repeat_u;
	v = v * repeat_v;
	
	// Bilinear filtering with horizontal wrapping and vertical clamping
	u = u - std::floor(u);
	v = v - std::floor(v);

	float x = u * float(imageWidth) - 0.5f;
	float y = (1.0f - v) * float(imageHeight) - 0.5f;

	int x0 = int(std::floor(x));
	int y0 = int(std::floor(y));
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	float fx = x - float(x0);
	float fy = y - float(y0);

	x0 = (x0 % imageWidth + imageWidth) % imageWidth;
	x1 = (x1 % imageWidth + imageWidth) % imageWidth;
	y0 = (y0 % imageHeight + imageHeight) % imageHeight;
	y1 = (y1 % imageHeight + imageHeight) % imageHeight;

	Uint8 r, g, b;
	Uint32 val00 = getpixel(surface, x0, y0);
	SDL_GetRGB(val00, surface->format, &r, &g, &b);
	color c00(r / 255.0f, g / 255.0f, b / 255.0f);

	Uint32 val10 = getpixel(surface, x1, y0);
	SDL_GetRGB(val10, surface->format, &r, &g, &b);
	color c10(r / 255.0f, g / 255.0f, b / 255.0f);

	Uint32 val01 = getpixel(surface, x0, y1);
	SDL_GetRGB(val01, surface->format, &r, &g, &b);
	color c01(r / 255.0f, g / 255.0f, b / 255.0f);

	Uint32 val11 = getpixel(surface, x1, y1);
	SDL_GetRGB(val11, surface->format, &r, &g, &b);
	color c11(r / 255.0f, g / 255.0f, b / 255.0f);

	color c0 = (1.0f - fx) * c00 + fx * c10;
	color c1 = (1.0f - fx) * c01 + fx * c11;

	return (1.0f - fy) * c0 + fy * c1;
}


class environment_texture : public image_texture
{
public:
	environment_texture() = default;
	environment_texture(const char* filename, float _zoom = 2.0f, float _rotation_y = 0.0f, float _pitch_offset = 0.0f)
		: image_texture(filename), zoom(_zoom), rotation_y(_rotation_y), pitch_offset(_pitch_offset) {
		estimate_sun_direction();
	}

	float zoom = 2.0f;          // Zoom factor: > 1.0 zooms out to reveal wider panorama in narrow FOV
	float rotation_y = 0.0f;    // Azimuth rotation around Y (in radians)
	float pitch_offset = 0.0f;  // Vertical pitch offset to adjust horizon level

	color value_from_direction(const vec3& dir) const {
		if (surface == nullptr) return color(1.0f, 0.0f, 1.0f);
		vec3 d = unit_vector(dir);
		float theta = std::acos(std::clamp(d.y(), -1.0f, 1.0f));
		float phi = std::atan2(d.z(), d.x());
		float u = (phi + pi) / (2.0f * pi);
		float v = 1.0f - (theta / pi);
		return value(u, v, point3(0.0f, 0.0f, 0.0f));
	}

	vec3 sun_direction = vec3(0.0f, 1.0f, 0.0f);
	int sun_pixel_x = 0;
	int sun_pixel_y = 0;

	// Convert (px, py) pixel coordinates to unit 3D direction vector
	vec3 pixel_to_direction(float px, float py) const {
		if (imageWidth <= 0 || imageHeight <= 0) return vec3(0.0f, 1.0f, 0.0f);
		float u = (px + 0.5f) / float(imageWidth);
		float v = 1.0f - (py + 0.5f) / float(imageHeight);
		float theta = (1.0f - v) * pi;
		float phi = (u - 0.5f) * 2.0f * pi + rotation_y;
		float sin_theta = std::sin(theta);
		return unit_vector(vec3(sin_theta * std::sin(phi), std::cos(theta), -sin_theta * std::cos(phi)));
	}

	// Angular error in degrees between estimated sun and a known reference direction
	float angular_error_degrees(const vec3& true_dir) const {
		float d = dot(unit_vector(true_dir), unit_vector(sun_direction));
		d = std::clamp(d, -1.0f, 1.0f);
		return std::acos(d) * (180.0f / pi);
	}

	// Angular error in degrees between estimated sun and known true pixel (e.g. from inspection or metadata)
	float angular_error_from_pixel(float true_px, float true_py) const {
		vec3 true_dir = pixel_to_direction(true_px, true_py);
		return angular_error_degrees(true_dir);
	}

	// Computes ground-truth sun direction using weighted barycenter/centroid of brightest pixels (Slide 9 formula)
	vec3 compute_ground_truth_sun(float& out_true_x, float& out_true_y, float threshold_ratio = 0.98f) const {
		if (!surface || imageWidth <= 0 || imageHeight <= 0) {
			out_true_x = out_true_y = 0.0f;
			return vec3(0.0f, 1.0f, 0.0f);
		}

		float max_lum = 0.0f;
		for (int y = 0; y < imageHeight; ++y) {
			for (int x = 0; x < imageWidth; ++x) {
				max_lum = std::max(max_lum, luminance_at(x, y));
			}
		}






		float sum_x = 0.0f, sum_y = 0.0f, total_weight = 0.0f;
		float min_lum = max_lum * threshold_ratio;
		for (int y = 0; y < imageHeight; ++y) {
			for (int x = 0; x < imageWidth; ++x) {
				float l = luminance_at(x, y);
				if (l >= min_lum) {
					float w = l - min_lum + 1e-4f;
					sum_x += x * w;
					sum_y += y * w;
					total_weight += w;
				}
			}
		}

		if (total_weight > 0.0f) {
			out_true_x = sum_x / total_weight;
			out_true_y = sum_y / total_weight;
		} else {
			out_true_x = 0.0f;
			out_true_y = 0.0f;
		}

		return pixel_to_direction(out_true_x, out_true_y);
	}

private:
	float luminance_at(int x, int y) const {
		if (!surface) return 0.0f;
		x = (x % imageWidth + imageWidth) % imageWidth;
		y = std::clamp(y, 0, imageHeight - 1);
		Uint32 pixel = getpixel(surface, x, y);
		Uint8 r, g, b;
		SDL_GetRGB(pixel, surface->format, &r, &g, &b);
		return 0.222f * r + 0.707f * g + 0.071f * b;
	}

	void estimate_sun_direction()
	{
		if (!surface || imageWidth <= 0 || imageHeight <= 0) return;

		int best_overall_x = 0;
		int best_overall_y = 0;
		float max_found_lum = -1.0f;

		// Stochastic restarts to avoid local traps
		const int restarts = 20;
		for (int r = 0; r < restarts; ++r) {
			int curX = int(random_float(0.0f, 1.0f) * float(imageWidth));
			int curY = int(random_float(0.0f, 1.0f) * float(imageHeight));
			curX = std::clamp(curX, 0, imageWidth - 1);
			curY = std::clamp(curY, 0, imageHeight - 1);

			float curValue = luminance_at(curX, curY);

			// Gradient ascent: search 8-neighborhood for strictly higher values
			while (true) {
				int nextX = curX;
				int nextY = curY;
				float nextValue = curValue;

				for (int dy = -1; dy <= 1; ++dy) {
					for (int dx = -1; dx <= 1; ++dx) {
						if (dx == 0 && dy == 0) continue;
						int nx = (curX + dx + imageWidth) % imageWidth;
						int ny = std::clamp(curY + dy, 0, imageHeight - 1);
						float val = luminance_at(nx, ny);
						if (val > nextValue) {
							nextValue = val;
							nextX = nx;
							nextY = ny;
						}
					}
				}

				// Stop when looking around gives only lower or equal values
				if (nextValue <= curValue) {
					break;
				}

				curX = nextX;
				curY = nextY;
				curValue = nextValue;
			}

			if (curValue > max_found_lum) {
				max_found_lum = curValue;
				best_overall_x = curX;
				best_overall_y = curY;
			}
		}

		sun_pixel_x = best_overall_x;
		sun_pixel_y = best_overall_y;

		// Convert (x, y) to spherical coordinates (theta, phi) and unit direction
		float u = (best_overall_x + 0.5f) / float(imageWidth);
		float v = 1.0f - (best_overall_y + 0.5f) / float(imageHeight);
		float theta = (1.0f - v) * pi;
		float phi = (u - 0.5f) * 2.0f * pi;
		float sin_theta = std::sin(theta);

		// Consistent with camera.h: phi = atan2(x, -z) -> x = sin_theta * sin(phi), z = -sin_theta * cos(phi)
		phi += rotation_y;
		sun_direction = unit_vector(vec3(sin_theta * std::sin(phi), std::cos(theta), -sin_theta * std::cos(phi)));
	}
};
