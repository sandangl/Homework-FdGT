#ifndef SPHERE_H
#define SPHERE_H

#include "interval.h"
#include "hittable.h"
#include "material.h"
#include "vec3.h"

class sphere : public hittable {
public:
  material* mat = nullptr;

  sphere() {
    center = point3(0.0f, 0.0f, 0.0f);
    radius = 1.0f;
  }
  sphere(point3 _center, float _radius, material* _mat = nullptr) : center(_center), radius(_radius), mat(_mat) {}

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
    
    float theta = std::acos(clamp(outward_normal.y(), -1.0f, 1.0f));
    float phi = std::atan2(outward_normal.z(), outward_normal.x());
    rec.u = -(phi + pi)/(2.0f*pi);
    rec.v = 1.0f - theta/pi;
    
    if (mat) rec.m = mat;

    return true;
  }

  bool hit_shadow(const ray& r, interval ray_t) const override {
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

    return true;
  }

private:
  point3 center;
  float radius;
};

#endif
