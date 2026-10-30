#pragma once
#include <algorithm>
#include <common.h>
#include <light.h>

#include "light_list.h"
#include "vec3.h"
#include "color.h"
#include "tinybvh/tiny_bvh.h"

class ext_light
{
public:
    point3 Q;
    vec3 u;
    vec3 v;
    int num_samples;

    color ambient;
    color diffuse;
    color specular;

    ext_light() {}

    ext_light(const point3& Q, const vec3& u, const vec3& v, color a, color d, color s, int num_samples = 50)
    {
        this->Q = Q;
        this->u = u;
        this->v = v;
        this->num_samples = num_samples;
        ambient = a;
        diffuse = d;
        specular = s;

        Q2 = this->Q + this->u;
        Q3 = this->Q2 + this->v;
        Q4 = this->Q + this->v;

        minQ = min_coord();
        maxQ = max_coord();
    }

    virtual ~ext_light() = default;

    virtual light_list getSources() = 0;

    point3 Q2;
    point3 Q3;
    point3 Q4;
    vec3 minQ;
    vec3 maxQ;

    vec3 min_coord()
    {
        return vvmin(Q, vvmin(Q2, vvmin(Q3, Q4)));
    }

    vec3 max_coord()
    {
        return vvmax(Q, vvmax(Q2, vvmax(Q3, Q4)));
    }
};

class point_ext_light : public ext_light
{
public:
    point_ext_light() {}
    point_ext_light(const point3& Q, const vec3& u, const vec3& v, color a, color d, color s, int num_samples = 50)
        : ext_light(Q, u, v, a, d, s, num_samples)
    {}

    light_list getSources() override
    {
        light_list lights;
        for (int i = 0; i < num_samples; i++)
        {
            point3 sample_pos = Q + random_float() * u + random_float() * v;
            point_light* sample = new point_light(sample_pos, ambient, diffuse, specular);
            lights.add(sample);
        }
        return lights;
    }
};

inline void light_list::add(ext_light* extended) {
    if (extended) add(extended->getSources());
}

inline void light_list::add(ext_light& extended) {
    add(extended.getSources());
}
