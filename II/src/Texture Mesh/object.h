#pragma once
#include "vec3.h"
#include "material.h"
#include "ray.h"
#include "aabb.h"
#include "hittable.h"

class object {
public:
	object() {}

	virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
	virtual bool hit_shadow(const ray& r, float t_min, float t_max) const = 0;

	virtual bool bounding_box(aabb& box) const = 0;
};
