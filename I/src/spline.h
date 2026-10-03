#ifndef SPLINE_H
#define SPLINE_H

#include "matrix.h"
#include "point3_list.h"

// catmull-rom spline

inline path generate_complete_CR_spline(const float dt, const matrix<4,4>& M_CR, const point3_list& positions) {
    // Every 4 consecutive positions make one segment, from the 2nd to the 3rd of them.
    path pathPoints{};
    int segments = (int)positions.points.size() - 3;
    for (int i = 0; i < segments; i++) {
        matrix<4, 3> controlSubset{
            fromVec3(positions.points[i]),
            fromVec3(positions.points[i + 1]),
            fromVec3(positions.points[i + 2]),
            fromVec3(positions.points[i + 3])
        };
        for (float t = 0.0f; t < 1.0f; t += dt) {
            vec<4> T{ t * t * t, t * t, t, 1 };
            pathPoints.add(toVec3(T * M_CR * controlSubset));
        }
    }
    return pathPoints;
}

inline path catmull_rom_interpolate(const point3_list& controlSet, const float dt) {
   // Computes a general catmull-rom set of points given a list of points:
   // one point per segment, all at the same t (t = 0 is P_i, t = 1 is P_i+1).

   // To create a closed curve, we need to duplicate some points:
   // The last one in first position, to make camera start at P_0,
   // and the first two at last position, to make camera finish at the starting point.

   int size = (int)controlSet.points.size();

   point3_list positions{ controlSet.points.back() };
   positions.add(controlSet);
   positions.add(controlSet.points[0]);
   positions.add(controlSet.points[1 % size]); // With a single point, 1 % size is 0, so that point is used everywhere.

   matrix<4,4> M_CR = 0.5f * matrix<4, 4>{{{-1, 3, -3, 1}, { 2, -5, 4, -1 }, { -1, 0, 1, 0 }, { 0, 2, 0, 0 }}};

   return generate_complete_CR_spline(dt, M_CR, positions);
}

#endif
