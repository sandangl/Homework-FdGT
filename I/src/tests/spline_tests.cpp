// Small tests for spline.h, written with the standard library only.

#include "test.h"
#include "../spline.h"

// A unit square, counter-clockwise.
static point3_list square() {
  point3_list l;
  l.add(point3(0, 0, 0));
  l.add(point3(1, 0, 0));
  l.add(point3(1, 1, 0));
  l.add(point3(0, 1, 0));
  return l;
}

static void expect(const path& got, const point3_list& want) {
  expect(got.points.size(), want.points.size());
  for (std::size_t i = 0; i < got.points.size() && i < want.points.size(); i++)
    expect(fromVec3(got.points[i]), fromVec3(want.points[i]));
}

static void spline_one_point_per_segment() {
  expect(catmull_rom_interpolate(square(), 0.5f).points.size(), 4);
}

static void spline_t0_gives_control_points() {
  // Segment i starts at P_i.
  expect(catmull_rom_interpolate(square(), 0), square());
}

static void spline_t1_closes_loop() {
  // Segment i ends at P_i+1, and the last segment ends back at P_0.
  point3_list want;
  want.add(point3(1, 0, 0));
  want.add(point3(1, 1, 0));
  want.add(point3(0, 1, 0));
  want.add(point3(0, 0, 0));
  expect(catmull_rom_interpolate(square(), 1), want);
}

static void spline_square_midpoint() {
  // Halfway between P_0 and P_1 the curve bulges slightly outside the square.
  path p = catmull_rom_interpolate(square(), 0.5f);
  expect(fromVec3(p.points[0]), vec<3>{0.5f, -0.125f, 0});
}

static void spline_single_point() {
  point3_list l;
  l.add(point3(1, 2, 3));
  expect(catmull_rom_interpolate(l, 0.5f), l);
}

int run_spline_tests(const char* filter) {
  int failed = 0;

  failed += run("spline_one_point_per_segment", spline_one_point_per_segment, filter);
  failed += run("spline_t0_gives_control_points", spline_t0_gives_control_points, filter);
  failed += run("spline_t1_closes_loop", spline_t1_closes_loop, filter);
  failed += run("spline_square_midpoint", spline_square_midpoint, filter);
  failed += run("spline_single_point", spline_single_point, filter);

  return failed;
}
