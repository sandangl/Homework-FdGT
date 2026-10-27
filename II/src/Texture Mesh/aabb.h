#pragma once
#include "vec3.h"
#include "ray.h"
#include "common.h"
#include "interval.h"

class aabb {
public:
	aabb() {}
	//aabb(const point3& a, const point3& b) { pmin = a; pmax = b; }
	aabb(const interval& ix, const interval& iy, const interval& iz)
		: x(ix), y(iy), z(iz) { }

	aabb(const point3& a, const point3& b) {
		// Treat the two points a and b as extrema for the bounding box, so we don't require a
		// particular minimum/maximum coordinate order.
		x = interval(fmin(a[0], b[0]), fmax(a[0], b[0]));
		y = interval(fmin(a[1], b[1]), fmax(a[1], b[1]));
		z = interval(fmin(a[2], b[2]), fmax(a[2], b[2]));
	}

	const interval& axis(int n) const {
		if (n == 1) return y;
		if (n == 2) return z;
		return x;
	}

	bool hit(const ray& r, interval ray_t) const {
		for (int a = 0; a < 3; a++) {
			auto invD = 1 / r.direction()[a];
			auto orig = r.origin()[a];

			auto t0 = (axis(a).min - orig) * invD;
			auto t1 = (axis(a).max - orig) * invD;

			if (invD < 0)
				std::swap(t0, t1);

			if (t0 > ray_t.min) ray_t.min = t0;
			if (t1 < ray_t.max) ray_t.max = t1;

			if (ray_t.max <= ray_t.min)
				return false;
		}
		return true;
	}

	//point3 min() const { return pmin; }
	//point3 max() const { return pmax; }
	/*
	bool hit(const ray& r, interval ray_t) const {
		for (int a = 0; a < 3; a++) {
			float t0 = ffmin((pmin[a] - r.origin()[a]) / r.direction()[a],
				(pmax[a] - r.origin()[a]) / r.direction()[a]);
			float t1 = ffmax((pmin[a] - r.origin()[a]) / r.direction()[a],
				(pmax[a] - r.origin()[a]) / r.direction()[a]);
			ray_t.min = ffmax(t0, ray_t.min);
			ray_t.max = ffmin(t1, ray_t.max);
			if (ray_t.max <= ray_t.min)
				return false;
		}
		return true;
	}

	point3 pmin;
	point3 pmax;
	*/

	interval x, y, z;
};

aabb surrounding_box(aabb box0, aabb box1) {
	point3 small(ffmin(box0.x.min, box1.x.min),
		ffmin(box0.y.min, box1.y.min),
		ffmin(box0.z.min, box1.z.min));
	point3 big(ffmax(box0.x.max, box1.x.max),
		ffmax(box0.y.max, box1.y.max),
		ffmax(box0.z.max, box1.z.max));
	return aabb(small, big);
}

