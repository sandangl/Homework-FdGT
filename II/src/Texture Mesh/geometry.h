#pragma once
#include "hittable.h"
#include "math.h"
#include "vec3.h"
#include "material.h"

class plane : public hittable {
public:
	point3 point;
	vec3 normal;
	material* mat = nullptr;

	plane() {}

	plane(const point3 p, const vec3 &n, material* m = nullptr) {
		point = p;
		normal = unit_vector(n);
		mat = m;
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		float denom = dot(r.direction(), normal);
		if (fabs(denom) > 1e-6f) {
			float t = dot(point - r.origin(), normal) / denom;
			if (ray_t.surrounds(t)) {
				rec.t = t;
				rec.p = r.at(t);
				rec.set_face_normal(r, normal);
				rec.m = mat;
				// Stretch texture across visible room span (X: [-50, 50], Y: [-40, 20], Z: [-128, -28])
				float u_raw = (fabs(normal.x()) > 0.5f) ? rec.p.z() : rec.p.x();
				rec.u = (u_raw + 50.0f) / 100.0f;
				rec.v = (fabs(normal.y()) > 0.5f) ? (rec.p.z() + 128.0f) / 100.0f : (rec.p.y() + 40.0f) / 60.0f;
				return true;
			}
		}
		return false;
	}

	bool hit_shadow(const ray& r, interval ray_t) const override {
		float denom = dot(r.direction(), normal);
		if (fabs(denom) > 1e-6f) {
			float t = dot(point - r.origin(), normal) / denom;
			if (ray_t.surrounds(t)) {
				return true;
			}
		}
		return false;
	}
};
