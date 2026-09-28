#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "interval.h"
#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
public:
  sphere(point3 _center, float _radius) : center(_center), radius(_radius) {}

  bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
    vec3 oc = r.origin() - center;
    float a = r.direction().length_squared();
    float half_b = dot(oc, r.direction());
    float c = oc.length_squared() - radius * radius;

    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0) return false;
    float sqrtd = sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range.
    float root = (-half_b - sqrtd) / a;
    if (!ray_t.surrounds(root)) {
      root = (-half_b + sqrtd) / a;
      if (!ray_t.surrounds(root))
        return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    vec3 outward_normal = (rec.p - center) / radius;
    rec.set_face_normal(r, outward_normal);

    return true;
  }

private:
  point3 center;
  float radius;
};

#endif


