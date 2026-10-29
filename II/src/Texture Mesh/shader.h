#pragma once
#include <iostream>
#include <time.h>
#include <algorithm>
#include "camera.h"
#include "light.h"
#include "material.h"
#include "object.h"
#include "color.h"
#include "vec3.h"
#include "math.h"
#include "hittable.h"

inline vec3 reflect(const vec3& i, const vec3& n)
{
	//return i - 2.0f * dot(i, n) * n;
	return  2.0f * dot(i, n) * n - i; // i vettore uscente dalla normale
}

color phong_shading(light& light, hit_record& hr, point3& camera_pos) {
	color ambient(0.0f, 0.0f, 0.0f);
	color diffuse(0.0f, 0.0f, 0.0f);
	color specular(0.0f, 0.0f, 0.0f);

	ambient = hr.m->ka * light.ambient;

	vec3 L = unit_vector(light.position - hr.p);
	float LDotN = std::max(dot(L, hr.normal), 0.0f);

	if (LDotN > 0.0f) {
		// Il coefficiente diffuso e' il colore della texture nel punto colpito, se il materiale
		// ne ha una; altrimenti il kd costante del materiale.
		color kd = (hr.m->tex != nullptr) ? hr.m->tex->value(hr.u, hr.v, hr.p) : hr.m->kd;
		diffuse = kd * light.diffuse * LDotN;

		vec3 R = reflect(L, hr.normal);

		vec3 V = unit_vector(camera_pos - hr.p);
		float VDotR = std::pow(std::max(dot(V, R), 0.0f), hr.m->alpha);

		specular = hr.m->ks * light.specular * VDotR;
		
		float f = light.calculate_light_interaction(hr.p);

		return ambient + f * (diffuse + specular);
	}
	else
		return ambient;
}

color ambient_shading(light &light, hit_record &hr) {
	color ambient(0.0f, 0.0f, 0.0f);
	
	ambient = hr.m->ka * light.ambient;

	return ambient;
}
