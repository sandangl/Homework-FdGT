#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <limits>
#include <memory>

// Common Headers
#include "ray.h"
#include "vec3.h"


// Usings

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// Constants

const float infinity = std::numeric_limits<float>::infinity();
const float pi = 3.14159f;

// Utility Functions

inline float degrees_to_radians(float degrees) {
  return degrees * pi / 180.0f;
}

inline float random_float() {
  // Returns a random real in [0,1).
  return float(rand() / (RAND_MAX + 1.0f));
}

inline float random_float(float min, float max) {
  // Returns a random real in [min,max).
  return min + (max - min) * random_float();
}

// lerp function

inline float lerp(float a, float b, float t) {
  return (1.0f - t) * a + t * b;
}

inline vec3 lerp(vec3& a, vec3& b, float t) {
  return (1.0f - t) * a + t * b;
}

inline float ffmin(float a, float b) { return a < b ? a : b; }
inline float ffmax(float a, float b) { return a > b ? a : b; }

inline vec3 vvmax(vec3 a, vec3 b)
{
  return vec3(ffmax(a.x(), b.x()), ffmax(a.y(), b.y()), ffmax(a.z(), b.z()));
}
inline vec3 vvmin(vec3 a, vec3 b)
{
  return vec3(ffmin(a.x(), b.x()), ffmin(a.y(), b.y()), ffmin(a.z(), b.z()));
}

#endif