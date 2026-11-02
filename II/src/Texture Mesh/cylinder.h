#pragma once
#include <cmath>
#include "vec3.h"
#include "hittable.h"
#include "interval.h"

class cylinder : public hittable
{
public:
    material* mat = nullptr;

    cylinder() : y0(-1.0f), y1(1.0f), radius(1.0f), inv_radius(1.0f)
    {
    }

    cylinder(const float bottom, const float top, const float r, material* m = nullptr)
        : y0(bottom), y1(top), radius(r), inv_radius(1.0f / r), mat(m)
    {
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override;
    bool hit_shadow(const ray& r, interval ray_t) const override;

private:
    float y0; // bottom y value
    float y1; // top y value
    float radius; // radius
    float inv_radius; // one over the radius	
};

inline bool cylinder::hit(const ray& ray, interval ray_t, hit_record& rec) const
{
    float temp;
    float ox = ray.o[0];
    float oy = ray.o[1];
    float oz = ray.o[2];
    float dx = ray.d[0];
    float dy = ray.d[1];
    float dz = ray.d[2];

    float a = dx * dx + dz * dz;
    float b = 2.0f * (ox * dx + oz * dz);
    float c = ox * ox + oz * oz - radius * radius;
    float disc = b * b - 4.0f * a * c;

    if (disc < 0.0f)
        return false;

    float e = sqrt(disc);
    float denom = 2.0f * a;
    if (std::fabs(denom) < 1e-8f)
        return false;

    if (std::fabs(dy) > 1e-6f)
    {
        float top_cap = (y1 - oy) / dy;
        float bot_cap = (y0 - oy) / dy;
        if (top_cap > ray_t.min && top_cap < ray_t.max)
        {
            point3 p = ray.at(top_cap);
            float xhit = ox + top_cap * dx;
            float zhit = oz + top_cap * dz;
            if (xhit * xhit + zhit * zhit <= radius * radius)
            {
                rec.normal = vec3(0, 1, 0);
                rec.p = p;
                rec.u = 0.5f + rec.p.x() * inv_radius * 0.5f;
                rec.v = 0.5f + rec.p.z() * inv_radius * 0.5f;
                rec.t = top_cap;
                if (mat) rec.m = mat;
                return true;
            }
        }
        else if (bot_cap > ray_t.min && bot_cap < ray_t.max)
        {
            point3 p = ray.at(bot_cap);
            float xhit = ox + bot_cap * dx;
            float zhit = oz + bot_cap * dz;
            if (xhit * xhit + zhit * zhit <= radius * radius)
            {
                rec.normal = vec3(0, -1, 0);
                rec.p = p;
                rec.u = 0.5f - rec.p.x() * inv_radius * 0.5f;
                rec.v = 0.5f + rec.p.z() * inv_radius * 0.5f;
                rec.t = bot_cap;
                if (mat) rec.m = mat;
                return true;
            }
        }
    }

    temp = (-b - e) / denom; // smaller root
    if (ray_t.surrounds(temp))
    {
        float yhit = oy + temp * dy;
        if (yhit > y0 && yhit < y1)
        {
            rec.t = temp;
            rec.normal = unit_vector(vec3((ox + temp * dx) * inv_radius, 0.0f, (oz + temp * dz) * inv_radius));
            rec.p = ray.at(rec.t);
            if (dot(-ray.d, rec.normal) < 0.0f)
                rec.normal = -rec.normal;
            rec.u = 0.5f + std::atan2(rec.normal.x(), rec.normal.z()) / (2.0f * pi);
            rec.v = (rec.p.y() - y0) / (y1 - y0);
            if (mat) rec.m = mat;
            return true;
        }
    }

    temp = (-b + e) / denom; // larger root
    if (ray_t.surrounds(temp))
    {
        float yhit = oy + temp * dy;
        if (yhit > y0 && yhit < y1)
        {
            rec.t = temp;
            rec.normal = unit_vector(vec3((ox + temp * dx) * inv_radius, 0.0f, (oz + temp * dz) * inv_radius));
            rec.p = ray.at(rec.t);
            if (dot(-ray.d, rec.normal) < 0.0f)
                rec.normal = -rec.normal;
            rec.u = 0.5f + std::atan2(rec.normal.x(), rec.normal.z()) / (2.0f * pi);
            rec.v = (rec.p.y() - y0) / (y1 - y0);
            if (mat) rec.m = mat;
            return true;
        }
    }

    return false;
}

inline bool cylinder::hit_shadow(const ray& ray, interval ray_t) const
{
    float temp;
    float ox = ray.o[0];
    float oy = ray.o[1];
    float oz = ray.o[2];
    float dx = ray.d[0];
    float dy = ray.d[1];
    float dz = ray.d[2];

    float a = dx * dx + dz * dz;
    float b = 2.0f * (ox * dx + oz * dz);
    float c = ox * ox + oz * oz - radius * radius;
    float disc = b * b - 4.0f * a * c;

    if (disc < 0.0f)
        return false;

    float e = sqrt(disc);
    float denom = 2.0f * a;
    if (std::fabs(denom) < 1e-8f)
        return false;

    temp = (-b - e) / denom;
    if (ray_t.surrounds(temp))
    {
        float yhit = oy + temp * dy;
        if (yhit > y0 && yhit < y1)
            return true;
    }

    temp = (-b + e) / denom;
    if (ray_t.surrounds(temp))
    {
        float yhit = oy + temp * dy;
        if (yhit > y0 && yhit < y1)
            return true;
    }

    return false;
}
