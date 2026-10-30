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
	
	virtual float calculate_light_interaction(point3 p)
	{
		vec3 l = unit_vector(p - position);
		return calculate_fspot(l);
	}

private:
	virtual float calculate_fspot(vec3 l)
	{
		return 1.0f;
	}
};

class spotlight : public light{
public:
	point3 p_s;
	vec3 s;
	float theta_0;
	float theta_1;
	float gradient_rate;
	
	spotlight() {}
	spotlight(point3 p_s, vec3 direction, float aperture, float smoothness, float gradient = 2.0f)
		: spotlight(p_s, direction, aperture, smoothness, color(0.05f, 0.05f, 0.05f), color(0.9f, 0.9f, 0.9f), color(0.5f, 0.5f, 0.5f), gradient) {}

	spotlight(point3 p_s, vec3 direction, float aperture, float smoothness, color a, color d, color s_col, float gradient = 2.0f)
	{
		position = p_s;
		this->p_s = p_s;
		s = unit_vector(direction);
		theta_0 = aperture;
		theta_1 = tinybvh::tinybvh_clamp(smoothness, 0.0f, 1.0f) * theta_0;
		gradient_rate = gradient;
		ambient = a;
		diffuse = d;
		specular = s_col;
	}
	
private:
	float calculate_fspot(vec3 l) override
	{	
		float cos_theta = dot(l, s);
		float cos_1 = cos(degrees_to_radians(theta_1));
		float cos_0 = cos(degrees_to_radians(theta_0));
		float t = 1;
		
		if (theta_0 != theta_1) {
			t = (cos_theta - cos_0) / (cos_1 - cos_0);
		}
		
		if (cos_theta > cos_1) { 
			return t;	//inner cone: linear decay
		}
		if (cos_theta > cos_0) {
			return pow(t, gradient_rate);	//outer cone: exponential decay
		}
		return 0.0f;
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
	
	float calculate_light_interaction(point3 p) override
	{
		return 1.0f;
	}
};
