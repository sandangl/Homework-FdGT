#ifndef CAMERA_H
#define CAMERA_H
// windows.h: NOMINMAX evita le macro min/max (che rompono std::max in shader.h);
// WIN32_LEAN_AND_MEAN esclude rpcndr.h, il cui "byte" e' ambiguo con std::byte (C++17)
// quando windows.h viene incluso dopo un "using namespace std".
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <ppl.h>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <vector>
#include "color.h"
#include "common.h"
#include "hittable_list.h"
#include "light.h"
#include "light_list.h"
#include "raster.h"
#include "shader.h"

using namespace std;
using namespace concurrency;

class camera
{
public:
    float aspect_ratio; // Ratio of image width over height
    int image_width; // Rendered image width in pixel count
    int image_height; // Rendered image height
    int samples_per_pixel; // Count of random samples for each pixel
    // Time instants
    float t0 = 0.0f;
    float t1 = 0.0f;
    float csi = 0.25f;

    float vfov = 90.0f; // Vertical view angle (field of view)
    point3 lookfrom = point3(0.0f, 0.0f, -1.0f); // Point camera is looking from
    point3 lookat = point3(0.0f, 0.0f, 0.0f); // Point camera is looking at
    vec3 vup = vec3(0.0f, 1.0f, 0.0f); // Camera-relative "up" direction
    point3 camera_center; // Camera center
    environment_texture* environment_texture_sky;

    camera()
    {
    }

    camera(float _aspect_ratio, int _image_width, float const _t1=0.0f, float const _t0=0.0f, float const _csi=0.25f)
    {
        aspect_ratio = _aspect_ratio;
        image_width = _image_width;

        // Calculate the image height, and ensure that it's at least 1.
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;
        
        this->t0 = _t0;
        this->t1 = _t1;
        this->csi = _csi;
    }

    void initialize()
    {
        // Calculate the image height, and ensure that it's at least 1.
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        if (t0 != t1 && csi > 0.0f) {
            if (csi > 1.0f) {
                // If csi was passed as number of samples/instants N (e.g. 48)
                samples_per_pixel = static_cast<int>(std::round(csi));
                csi = 1.0f / static_cast<float>(samples_per_pixel);
            } else {
                // If csi is step size Delta_xi (e.g. 0.25f or 1.0f / 48.0f)
                samples_per_pixel = std::max(1, static_cast<int>(std::round(1.0f / csi)));
            }
        } else if (samples_per_pixel <= 0) {
            samples_per_pixel = 1;
        }

        camera_center = lookfrom;

        // Determine viewport dimensions.
        float focal_length = (lookfrom - lookat).length();
        float theta = degrees_to_radians(vfov);
        float h = tan(theta / 2.0f);
        float viewport_height = 2.0f * h * focal_length;
        float viewport_width = viewport_height * (static_cast<float>(image_width) / image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u; // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v; // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        point3 viewport_upper_left = camera_center - (focal_length * w) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
    }

    void render(hittable_list& world, light& light, const int K = 3)
    {
        light_list lights(light);
        render(world, lights, K);
    }
    void parallel_render(hittable_list& world, light& light, const int K = 3)
    {
        light_list lights(light);
        parallel_render(world, lights, K);
    }

    void render(hittable_list& world, light_list& lights, const int K = 3)
    {
        for (int j = 0; j < image_height; ++j)
        {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; ++i)
            {
                color pixel_color(0.0f, 0.0f, 0.0f);

                for (int sample = 0; sample < samples_per_pixel; ++sample)
                {
                    float cur_csi = (samples_per_pixel > 1) ? std::min(1.0f, sample * csi) : 0.0f;
                    ray r = get_ray(i, j, cur_csi);
                    pixel_color += ray_color(r, world, lights, K);
                }

                pixel_color /= samples_per_pixel;
                setColor(pixel_color[0], pixel_color[1], pixel_color[2]);
                setPixel(i, j);
            }
        }
        SDL_RenderPresent(renderer);
    }

    void parallel_render(hittable_list& world, light_list& lights, const int K = 3)
    {
        vector<color> matrix(image_width * image_height);

        parallel_for(int(0), image_height, [&](int j)
        {
            for (int i = 0; i < image_width; ++i)
            {
                color pixel_color(0.0f, 0.0f, 0.0f);

                for (int sample = 0; sample < samples_per_pixel; ++sample)
                {
                    float cur_csi = (samples_per_pixel > 1) ? std::min(1.0f, sample * csi) : 0.0f;
                    ray r = get_ray(i, j, cur_csi);
                    pixel_color += ray_color(r, world, lights, K);
                }

                pixel_color /= samples_per_pixel;
                matrix[j * image_width + i] = pixel_color;
            }
        });

        for (int j = 0; j < image_height; j++)
        {
            for (int i = 0; i < image_width; i++)
            {
                color pixel_color = matrix[j * image_width + i];
                setColor(pixel_color[0], pixel_color[1], pixel_color[2]);
                setPixel(i, j);
            }
        }
        SDL_RenderPresent(renderer);
    }

private:
    point3 pixel00_loc; // Location of pixel 0, 0
    vec3 pixel_delta_u; // Offset to pixel to the right
    vec3 pixel_delta_v; // Offset to pixel below

    vec3 u, v, w; // Camera frame basis vectors

    ray get_ray(int i, int j, float cur_csi = 0.0f) const
    {
        // Get a randomly sampled camera ray for the pixel at location i,j.

        vec3 pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
        vec3 pixel_sample = pixel_center + pixel_sample_square();

        point3 ray_origin = camera_center;
        vec3 ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction, t0, t1, cur_csi);
    }

    vec3 pixel_sample_square() const
    {
        // Returns a random point in the square surrounding a pixel at the origin.
        float px = -0.5 + random_float();
        float py = -0.5 + random_float();
        return (px * pixel_delta_u) + (py * pixel_delta_v);
    }

    color ray_color(const ray& r, hittable_list& world, light_list& lights, const int K = 3)
    {
        hit_record rec;

        if (!world.hit(r, interval(0.001f, infinity), rec))
        {
            vec3 unit_direction = unit_vector(r.direction());
            
            if (environment_texture_sky != nullptr)
            {
                float theta = std::acos(tinybvh::tinybvh_clamp(unit_direction.y(), -1.0f, 1.0f));
                float phi = std::atan2(unit_direction.x(), -unit_direction.z());

                if (environment_texture_sky->zoom != 1.0f || environment_texture_sky->pitch_offset != 0.0f || environment_texture_sky->rotation_y != 0.0f)
                {
                    vec3 forward = unit_vector(lookat - lookfrom);
                    float theta_cam = std::acos(tinybvh::tinybvh_clamp(forward.y(), -1.0f, 1.0f));
                    float phi_cam = std::atan2(forward.x(), -forward.z());

                    float d_theta = theta - theta_cam;
                    float d_phi = phi - phi_cam;
                    while (d_phi > pi) d_phi -= 2.0f * pi;
                    while (d_phi < -pi) d_phi += 2.0f * pi;

                    theta = (pi / 2.0f) + d_theta * environment_texture_sky->zoom + environment_texture_sky->pitch_offset;
                    phi = phi_cam + d_phi * environment_texture_sky->zoom + environment_texture_sky->rotation_y;
                }

                float u = 0.5f + phi / (2.0f * pi);
                float v = 1.0f - theta / pi;
                return environment_texture_sky->value(u, v, unit_direction);
            }
            float t = 0.5f * (unit_direction.y() + 1.0f);
            return (1.0f - t) * color(1.0f, 1.0f, 1.0f) + t * color(0.5f, 0.7f, 1.0f);
        }

        // Toon contour check: if toon is enabled for this material and |n . v| < epsilon, pixel is black
        if (rec.m == nullptr || rec.m->is_toon)
        {
            vec3 V = unit_vector(camera_center - rec.p);
            float eps = (rec.m != nullptr) ? rec.m->epsilon : 0.26f;
            if (std::fabs(dot(V, rec.normal)) < eps)
            {
                return color(0.0f, 0.0f, 0.0f);
            }
        }

        color pixel_color(0.0f, 0.0f, 0.0f);
        int num_sources =  (int) lights.sources.size();
        for (light* source : lights.sources)
        {
            vec3 l = source->get_direction(rec.p);
            float closest_light = source->get_distance(rec.p);
            ray shadow_ray(rec.p, l, r.time());

            if (world.hit_shadow(shadow_ray, interval(0.01f, closest_light)))
                pixel_color += ambient_shading(*source, rec) /num_sources;
            else
                if (rec.m == nullptr || rec.m->is_toon)
                {
                    pixel_color += toon_shading(*source, rec, camera_center, K) / num_sources;
                } else
                {
                    pixel_color += phong_shading(*source, rec, camera_center) / num_sources;
                }
        }
        return pixel_color;
    }
};

#endif
