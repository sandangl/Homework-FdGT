#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <limits>
#include <memory>

// Common Headers
#include "ray.h"
#include "vec3.h"
#include "vec.h"


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
  return rand() / (RAND_MAX + 1.0f);
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


#endif