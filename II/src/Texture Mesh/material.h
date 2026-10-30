#pragma once
#include "color.h"
#include "light.h"
#include "common.h"
#include "texture.h"

class material {
public:
	color ka;
	color kd;
	color ks;
	float alpha;

	bool is_toon = false;
	int K = 3;
	float epsilon = 0.26f;

	// Texture opzionale: se e' presente sostituisce kd nel termine diffuso (vedi shader.h).
	// NB: il membro non puo' chiamarsi "texture" come il tipo (g++: "changes meaning of 'texture'").
	texture* tex = nullptr;

	material(color ambient, color diffuse, color specular, float a) : ka(ambient), kd(diffuse), ks(specular), alpha(a) {};

	material() {

		ka[0] = random_float();
		ka[1] = random_float();
		ka[2] = random_float();

		kd[0] = random_float();
		kd[1] = random_float();
		kd[2] = random_float();

		ks[0] = random_float();
		ks[1] = random_float();
		ks[2] = random_float();

		alpha = 20.0f + random_float() * 200.0f;
	}
};
