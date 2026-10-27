#pragma once
#include "vec3.h"
#include "color.h"

class light {
public:
	color ambient;
	color diffuse;
	color specular;

	float att_a;
	float att_b;
	float att_c;
};

class point_light : public light {
public:
	point3 position;

	point_light() {}
	point_light(point3 p, color a, color d, color s) {
		position = p;
		ambient = a;
		diffuse = d;
		specular = s;
	}
};
