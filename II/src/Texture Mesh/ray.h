#ifndef RAY_H
#define RAY_H

#include <algorithm>
#include <vector>

#include "vec3.h"

class ray
{
public:
	point3 o;
	vec3 d;
	float tau = 0.0f; // time instant
	float t0 = 0.0f;
	float t1 = 0.0f;
	float csi = 0.0f;

	ray() {}
	ray(const point3& _origin, const vec3& _direction, float _time = 0.0f)
		: o(_origin), d(_direction), tau(_time), t0(_time), t1(_time), csi(0.0f) {}

	ray(const point3& _origin, const vec3& _direction, float _t0, float _t1, float _csi)
		: o(_origin), d(_direction), t0(_t0), t1(_t1), csi(std::max(_csi, 0.0f))
	{
		this->tau = t0 + csi * (t1 - t0);
	}

	point3 origin() const { return o; }
	vec3 direction() const { return d; }
	float time() const { return tau; }
	point3 at(float t) const { return o + t * d; }
	float t_at_point(point3& p) const { return (p - o).length(); }
	point3 c_at_tau() const { return (t1 - t0 != 0.0f) ? (at(t0) + (tau - t0) / (t1 - t0) * (at(t1) - at(t0))) : at(tau); }
};

#endif
