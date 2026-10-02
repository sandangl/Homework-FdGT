#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <limits>
#include <memory>

// Common Headers
#include "ray.h"
#include "vec3.h"
#include "point3_list.h"
#include "vec.h"
#include "matrix.h"


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

// catmull-rom spline

inline path catmull_rom_interpolate(const point3_list& controlSet, const float t) {
   // Computes a general catmull-rom set of points given a list of points.

   // To create a closed curve, we need to duplicate some points:
   // The last one in first position, to make camera start at P_0,
   // and the first one at last position, to make camera finish at the starting point.

   point3_list positions{ controlSet.points.back };
   positions.add(controlSet.points);
   positions.add(controlSet.points.front);

   int posLength = positions.length;

   if (posLength < 4) {
     // Last point gets duplicated for Catmull-Rom to work.
     positions.points.add(positions.points.at(posLength - 1));
   }
   
   vec<4> T{ pow(t,3), pow(t,2), t, 1 };
   matrix<4,4> M_CR = 0.5f * matrix<4, 4>{{{-1, 3, -3, 1}, { 2, -5, 4, -1 }, { -1, 0, 1, 0 }, { 0, 2, 0, 0 }}};
   matrix<posLength, 3> P = fromPoint3List(positions);


   return generate_complete_CR_spline<posLength>(T, M_CR, P);
}

template <std::size_t N>
inline path generate_complete_CR_spline(const vec<4>& T, const matrix<4,4>& M_CR, const matrix<N, 3>& P) {
    float max = ceil( ((float)N) / 4.0f);
    matrix<max, 3> pathMatrix{};
    for (int i = 0; i < max; i++) {
        int accessIndex = 4 * i;
        matrix<4, 3> controlSubset{ P.rows[accessIndex], P.rows[accessIndex + 1], P.rows[accessIndex + 2], P.rows[accessIndex + 3] };
        path.rows[i] = T * M_CR * controlSubset;
    }
    return toPoint3List(pathMatrix);
}

#endif