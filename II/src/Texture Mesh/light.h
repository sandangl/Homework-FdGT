#pragma once
#include <algorithm>
#include <common.h>

#include "vec3.h"
#include "color.h"
#include "tinybvh/tiny_bvh.h"

class light {
public:
	point3 position;
	
	color ambient;
	color diffuse;
	color specular;

	float att_a;
	float att_b;
	float att_c;
	
	virtual ~light() = default;
	virtual float calculate_light_interaction(point3 p, float luminance) = 0;
};

class spotlight : public light{
public:
	point3 p_s;
	vec3 s;
	float theta_0;
	float theta_1;
	float gradient_rate;
	
	spotlight() {}
	spotlight(point3 p_s, vec3 direction, float aperture, float smoothness, float gradient_rate = 30)
	{
		position = p_s;
		s = direction;
		theta_0 = aperture;
		theta_1 = tinybvh::tinybvh_clamp(smoothness, 0.0f, 1.0f) * theta_0;
	}
	
	float calculate_light_interaction(point3 p, float luminance) override
	{
		vec3 p_ps = p - p_s;
		float dist = p_ps.length();
		vec3 l = (p_ps)/dist;
		float fspot = calculate_fspot(l, luminance); 
		return fspot;
	}
	
private:
	float calculate_fspot(vec3 l, float luminance)
	{
		if (dot(-l,s) > cos(degrees_to_radians(theta_0))) { return 0; }
		return tinybvh::tinybvh_clamp(luminance, 0.0f, 1.0f) * pow(dot(-l,s), gradient_rate);
	}
};

class point_light : public light {
public:
	point_light() {}
	point_light(point3 p, color a, color d, color s) {
		position = p;
		ambient = a;
		diffuse = d;
		specular = s;
	}
	
	float calculate_light_interaction(point3 p, float luminance) override
	{
		return tinybvh::tinybvh_clamp(luminance, 0.0f, 1.0f);
	}
	
};
