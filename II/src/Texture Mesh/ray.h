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
	float t0 = 0.0f;
	float t1 = 0.0f;
	float zeta = 0.25f;
	float tau = 0.0f;

	ray() {}
	ray(const point3& _origin, const vec3& _direction, const float _t1 = 0.0f, const float _t0 = 0.0f, const float _zeta = 0.25f)
	{
		o = _origin;
		d = _direction;
		this->zeta = std::max(_zeta, 10e-6f);
		this->t0 = _t0;
		this->t1 = _t1;
		this->tau = t0 + zeta * (t1 - t0);
	}
	point3 origin() const { return o; }
	vec3 direction() const { return d; }
	point3 at(float t) const { return o + t * d; }
	float t_at_point(point3& p) const { return (p - o).length(); }
	point3 c_at_tau() const { return (t1 - t0 != 0.0f) ? (at(t0) + (tau - t0) / (t1 - t0) * (at(t1) - at(t0))) : at(tau); }
};

#endif
