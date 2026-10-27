#pragma once
#include <math.h> 
#include "object.h"
#include "math.h"
#include "vec3.h"
#include "hittable.h"

class cylinder : public object {

public:

	cylinder() {}
	cylinder(const float bottom, const float top, const float r) : y0(bottom), y1(top), radius(r), inv_radius(1.0f / r) {};

	cylinder(const float bottom, const float top, const float r, const material* m) : y0(bottom), y1(top), radius(r), inv_radius(1.0f / r) {};

	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;
	virtual bool hit_shadow(const ray& r, float t_min, float t_max) const;

private:

	float		y0;						// bottom y value
	float		y1;						// top y value
	float		radius;				// radius
	float		inv_radius;  	// one over the radius	
};

bool cylinder::hit(const ray& ray, float t_min, float t_max, hit_record& rec) const {
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

	if (disc < 0.0)
		return(false);
	else {
		float e = sqrt(disc);
		float denom = 2.0f * a;
		temp = (-b - e) / denom;    // smaller root

		if (temp < t_max && temp > t_min) {
			float yhit = oy + temp * dy;

			if (yhit > y0 && yhit < y1) {
				rec.t = temp;
				rec.normal = unit_vector(vec3((ox + temp * dx) * inv_radius, 0.0f, (oz + temp * dz) * inv_radius));
				rec.p = ray.at(rec.t);
				//rec.m = mat;

				// test for hitting from inside
				if (dot(-ray.d, rec.normal) < 0.0f)
					rec.normal = -rec.normal;

				return (true);
			}
		}

		temp = (-b + e) / denom;    // larger root

		if (temp < t_max && temp > t_min) {
			float yhit = oy + temp * dy;

			if (yhit > y0 && yhit < y1) {
				rec.t = temp;
				rec.normal = unit_vector(vec3((ox + temp * dx) * inv_radius, 0.0f, (oz + temp * dz) * inv_radius));
				rec.p = ray.at(rec.t);
				//rec.m = mat;

				// test for hitting inside surface
				if (dot(-ray.d, rec.normal) < 0.0f)
					rec.normal = -rec.normal;

				return (true);
			}
		}
	}

	return (false);
}

bool cylinder::hit_shadow(const ray& ray, float t_min, float t_max) const {
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
		return(false);
	else {
		float e = sqrt(disc);
		float denom = 2.0f * a;
		temp = (-b - e) / denom;    // smaller root

		if (temp < t_max && temp > t_min) {
			float yhit = oy + temp * dy;

			if (yhit > y0 && yhit < y1) {
				//rec.t = temp;
				//rec.normal = normalize(vector3D((ox + t * dx) * inv_radius, 0.0, (oz + t * dz) * inv_radius));

				// test for hitting from inside

				//if (dot(-ray.d, rec.normal) < 0.0)
				//	rec.normal = -rec.normal;

				//rec.p = ray.point_at_parameter(rec.t);

				return (true);
			}
		}

		temp = (-b + e) / denom;    // larger root

		if (temp < t_max && temp > t_min) {
			float yhit = oy + temp * dy;

			if (yhit > y0 && yhit < y1) {
				//rec.t = temp;
				//rec.normal = normalize(vector3D((ox + t * dx) * inv_radius, 0.0, (oz + t * dz) * inv_radius));

				// test for hitting inside surface

				//if (dot(-ray.d, rec.normal) < 0.0)
				//	rec.normal = -rec.normal;

				//rec.p = ray.point_at_parameter(rec.t);

				return (true);
			}
		}
	}

	return (false);
}
