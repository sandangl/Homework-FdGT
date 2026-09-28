#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray
{
public:
	point3 o;
	vec3 d;

	ray() {}
	ray(const point3& _origin, const vec3& _direction) { o = _origin; d = _direction; }
	point3 origin() const { return o; }
	vec3 direction() const { return d; }
	point3 at(float t) const { return o + t * d; }
};

#endif
