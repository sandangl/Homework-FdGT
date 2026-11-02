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

class box : public hittable {
public:
	point3 pmin;
	point3 pmax;
	material* mat = nullptr;

	box() : pmin(point3(-0.5f, -0.5f, -0.5f)), pmax(point3(0.5f, 0.5f, 0.5f)) {}
	box(const point3& p0, const point3& p1, material* m = nullptr)
		: pmin(vvmin(p0, p1)), pmax(vvmax(p0, p1)), mat(m) {}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		float t_enter = ray_t.min;
		float t_exit = ray_t.max;
		vec3 enter_normal(0.0f, 0.0f, 0.0f);
		vec3 exit_normal(0.0f, 0.0f, 0.0f);

		for (int a = 0; a < 3; a++) {
			float dir_a = r.direction()[a];
			float orig_a = r.origin()[a];
			float min_a = pmin[a];
			float max_a = pmax[a];

			if (std::fabs(dir_a) < 1e-8f) {
				if (orig_a < min_a || orig_a > max_a)
					return false;
			} else {
				float invD = 1.0f / dir_a;
				float t0 = (min_a - orig_a) * invD;
				float t1 = (max_a - orig_a) * invD;

				vec3 n0(0.0f, 0.0f, 0.0f);
				n0[a] = -1.0f;
				vec3 n1(0.0f, 0.0f, 0.0f);
				n1[a] = 1.0f;

				if (invD < 0.0f) {
					std::swap(t0, t1);
					std::swap(n0, n1);
				}

				if (t0 > t_enter) {
					t_enter = t0;
					enter_normal = n0;
				}
				if (t1 < t_exit) {
					t_exit = t1;
					exit_normal = n1;
				}

				if (t_exit <= t_enter)
					return false;
			}
		}

		float t_hit = t_enter;
		vec3 hit_normal = enter_normal;
		if (t_enter < ray_t.min) {
			t_hit = t_exit;
			hit_normal = exit_normal;
			if (t_hit < ray_t.min || t_hit > ray_t.max)
				return false;
		}

		rec.t = t_hit;
		rec.p = r.at(rec.t);		
		rec.set_face_normal(r, hit_normal);
		
		vec3 d_box = pmax - pmin;

		if (std::fabs(hit_normal.x() - 1.0f) < 1e-4f) { // +X face
			rec.u = (pmax.z() - rec.p.z()) / d_box.z();
			rec.v = (rec.p.y() - pmin.y()) / d_box.y();
		} else if (std::fabs(hit_normal.x() + 1.0f) < 1e-4f) { // -X face
			rec.u = (rec.p.z() - pmin.z()) / d_box.z();
			rec.v = (rec.p.y() - pmin.y()) / d_box.y();
		} else if (std::fabs(hit_normal.y() - 1.0f) < 1e-4f) { // +Y (Top)
			rec.u = (rec.p.x() - pmin.x()) / d_box.x();
			rec.v = (pmax.z() - rec.p.z()) / d_box.z();
		} else if (std::fabs(hit_normal.y() + 1.0f) < 1e-4f) { // -Y (Bottom)
			rec.u = (rec.p.x() - pmin.x()) / d_box.x();
			rec.v = (rec.p.z() - pmin.z()) / d_box.z();
		} else if (std::fabs(hit_normal.z() - 1.0f) < 1e-4f) { // +Z (Front)
			rec.u = (rec.p.x() - pmin.x()) / d_box.x();
			rec.v = (rec.p.y() - pmin.y()) / d_box.y();
		} else { // -Z (Back)
			rec.u = (pmax.x() - rec.p.x()) / d_box.x();
			rec.v = (rec.p.y() - pmin.y()) / d_box.y();
		}
		
		if (mat) rec.m = mat;
		return true;
	}

	bool hit_shadow(const ray& r, interval ray_t) const override {
		float t_enter = ray_t.min;
		float t_exit = ray_t.max;

		for (int a = 0; a < 3; a++) {
			float dir_a = r.direction()[a];
			float orig_a = r.origin()[a];
			float min_a = pmin[a];
			float max_a = pmax[a];

			if (std::fabs(dir_a) < 1e-8f) {
				if (orig_a < min_a || orig_a > max_a)
					return false;
			} else {
				float invD = 1.0f / dir_a;
				float t0 = (min_a - orig_a) * invD;
				float t1 = (max_a - orig_a) * invD;
				if (invD < 0.0f) std::swap(t0, t1);

				if (t0 > t_enter) t_enter = t0;
				if (t1 < t_exit) t_exit = t1;

				if (t_exit <= t_enter)
					return false;
			}
		}

		return true;
	}
};

