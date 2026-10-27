#pragma once
#include "object.h"
#include "math.h"
#include "vec3.h"
#include "material.h"

class plane : public object {
public:
	plane() {}

	plane(const point3 p, const vec3 &n) {
		point = p;
		normal = n;
}

	virtual bool hit(const ray& r, float tmin, float tmax, hit_record &rec) const;

private:
	point3 point;
	vec3 normal;
};

bool plane::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	// t = ((point - o) . n) / (d . n): senza normalizzare d, altrimenti t non sarebbe
	// il parametro del raggio r(t) = o + t d. denom ~ 0: raggio parallelo al piano.
	float denom = dot(r.d, normal);
	if (fabs(denom) > 1e-6f) {
		float temp = dot(point - r.o, normal) / denom;
		if (temp < t_max && temp > t_min) {
			rec.t = temp;
			rec.p = r.at(rec.t);
			rec.normal = normal;
			return true;
		}
	}
	return false;
}
