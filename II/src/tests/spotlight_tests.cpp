// Tests for spotlight cone (theta_0 and theta_1) in light.h

#include "test.h"
#include "light.h"

// Helper to construct a point at a given angle (degrees) relative to spotlight direction (0, 0, 1)
static point3 make_sample_point(const point3& origin, float angle_deg, float distance = 5.0f, float azimuth_deg = 0.0f) {
  float theta = degrees_to_radians(angle_deg);
  float phi = degrees_to_radians(azimuth_deg);
  float x = distance * std::sin(theta) * std::cos(phi);
  float y = distance * std::sin(theta) * std::sin(phi);
  float z = distance * std::cos(theta);
  return origin + vec3(x, y, z);
}

static void test_initialization() {
  spotlight spot(point3(1, 2, 3), vec3(0, 0, 1), 30.0f, 0.5f, 20.0f);
  expect_near(spot.theta_0, 30.0f);
  expect_near(spot.theta_1, 15.0f);
  expect_near(spot.decaying_speed, 20.0f);
  expect_near(spot.p_s.x(), 1.0f);
  expect_near(spot.p_s.y(), 2.0f);
  expect_near(spot.p_s.z(), 3.0f);
}

static void test_smoothness_clamping() {
  // Negative smoothness clamps to 0.0f -> theta_1 = 0
  spotlight spot_neg(point3(0, 0, 0), vec3(0, 0, 1), 40.0f, -0.5f);
  expect_near(spot_neg.theta_1, 0.0f);

  // Smoothness > 1.0f clamps to 1.0f -> theta_1 = theta_0
  spotlight spot_over(point3(0, 0, 0), vec3(0, 0, 1), 40.0f, 1.5f);
  expect_near(spot_over.theta_1, 40.0f);

  // Exact 0.0 and 1.0
  spotlight spot_zero(point3(0, 0, 0), vec3(0, 0, 1), 40.0f, 0.0f);
  expect_near(spot_zero.theta_1, 0.0f);
  spotlight spot_one(point3(0, 0, 0), vec3(0, 0, 1), 40.0f, 1.0f);
  expect_near(spot_one.theta_1, 40.0f);
}

static void test_direction_normalization() {
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 10), 30.0f, 0.5f);
  expect_near(spot.s.length(), 1.0f);
  expect_near(spot.s.x(), 0.0f);
  expect_near(spot.s.y(), 0.0f);
  expect_near(spot.s.z(), 1.0f);
}

static void test_on_axis_peak_intensity() {
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.5f, 30.0f);
  // Point directly in front along cone axis
  point3 p(0, 0, 10);
  float f = spot.calculate_light_interaction(p);
  expect_near(f, 1.0f);
}

static void test_inner_cone_linear_decay() {
  // theta_0 = 30 deg, smoothness = 0.6 -> theta_1 = 18 deg
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.6f);

  // Sample at 10 degrees (< theta_1: inside inner cone -> full intensity 1.0)
  float angle = 10.0f;
  point3 p = make_sample_point(point3(0, 0, 0), angle, 5.0f);
  float f = spot.calculate_light_interaction(p);

  expect_near(f, 1.0f);
}

static void test_inner_cone_monotonic_decrease() {
  // theta_0 = 40 deg, smoothness = 0.5 -> theta_1 = 20 deg
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 40.0f, 0.5f);

  float f_center = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), 0.0f));
  float f_10deg  = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), 10.0f));
  float f_25deg  = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), 25.0f));
  float f_35deg  = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), 35.0f));

  expect_near(f_center, 1.0f);
  expect_near(f_10deg, 1.0f);
  expect_true(f_10deg > f_25deg, "10deg > 25deg");
  expect_true(f_25deg > f_35deg, "25deg > 35deg");
}

static void test_outer_cone_exponential_decay() {
  // theta_0 = 30 deg, smoothness = 0.5 -> theta_1 = 15 deg
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.5f);

  // Sample at 22.5 degrees (between theta_1=15 and theta_0=30)
  float angle = 22.5f;
  point3 p = make_sample_point(point3(0, 0, 0), angle, 5.0f);
  float f = spot.calculate_light_interaction(p);

  float cos_angle = std::cos(degrees_to_radians(angle));
  float cos_0 = std::cos(degrees_to_radians(30.0f));
  float cos_1 = std::cos(degrees_to_radians(15.0f));
  float expected = (cos_angle - cos_0) / (cos_1 - cos_0);
  expect_near(f, expected);
  expect_true(f > 0.0f && f < 1.0f, "outer cone attenuation is between 0 and 1");
}

static void test_outer_cone_boundary_cutoff() {
  // theta_0 = 30 deg, theta_1 = 15 deg
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.5f, 20.0f);

  // Just inside outer cone (29.9 deg) -> positive intensity
  point3 p_inside = make_sample_point(point3(0, 0, 0), 29.9f);
  float f_inside = spot.calculate_light_interaction(p_inside);
  expect_true(f_inside > 0.0f, "just inside outer cone should have non-zero light");

  // Just outside outer cone (30.1 deg) -> exactly 0.0f
  point3 p_outside = make_sample_point(point3(0, 0, 0), 30.1f);
  float f_outside = spot.calculate_light_interaction(p_outside);
  expect(f_outside, 0.0f);

  // Far outside cone (45.0 deg) -> exactly 0.0f
  point3 p_far = make_sample_point(point3(0, 0, 0), 45.0f);
  expect(spot.calculate_light_interaction(p_far), 0.0f);
}

static void test_sharp_boundary_when_smoothness_one() {
  // smoothness = 1.0 -> theta_1 == theta_0 == 25 deg (hard-edge spotlight)
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 25.0f, 1.0f);

  // Inside cone at 24.5 deg -> returns 1.0f
  point3 p_in = make_sample_point(point3(0, 0, 0), 24.5f);
  float f_in = spot.calculate_light_interaction(p_in);
  expect_near(f_in, 1.0f);

  // Outside cone at 25.5 deg -> returns 0.0f
  point3 p_out = make_sample_point(point3(0, 0, 0), 25.5f);
  expect(spot.calculate_light_interaction(p_out), 0.0f);
}

static void test_zero_smoothness_immediate_outer_cone() {
  // smoothness = 0.0 -> theta_1 = 0 deg
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.0f);

  // Off-axis at 15 deg -> outer cone decay applies immediately
  point3 p = make_sample_point(point3(0, 0, 0), 15.0f);
  float f = spot.calculate_light_interaction(p);
  float cos_angle = std::cos(degrees_to_radians(15.0f));
  float cos_0 = std::cos(degrees_to_radians(30.0f));
  float cos_1 = 1.0f;
  float expected = (cos_angle - cos_0) / (cos_1 - cos_0);
  expect_near(f, expected);
}

static void test_distance_invariance() {
  // calculate_light_interaction evaluates angular falloff only; distance attenuation is separate
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.5f, 20.0f);

  point3 p_d1  = make_sample_point(point3(0, 0, 0), 10.0f, 1.0f);
  point3 p_d5  = make_sample_point(point3(0, 0, 0), 10.0f, 5.0f);
  point3 p_d50 = make_sample_point(point3(0, 0, 0), 10.0f, 50.0f);

  float f1  = spot.calculate_light_interaction(p_d1);
  float f5  = spot.calculate_light_interaction(p_d5);
  float f50 = spot.calculate_light_interaction(p_d50);

  expect_near(f1, f5);
  expect_near(f5, f50);
}

static void test_radial_symmetry() {
  // Spot pointing along Z axis should have circular symmetry around Z
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.5f, 15.0f);

  float angle = 12.0f; // inside inner cone
  float f_0   = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), angle, 5.0f, 0.0f));
  float f_90  = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), angle, 5.0f, 90.0f));
  float f_180 = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), angle, 5.0f, 180.0f));
  float f_270 = spot.calculate_light_interaction(make_sample_point(point3(0, 0, 0), angle, 5.0f, 270.0f));

  expect_near(f_0, f_90);
  expect_near(f_90, f_180);
  expect_near(f_180, f_270);
}

static void test_perpendicular_and_behind() {
  spotlight spot(point3(0, 0, 0), vec3(0, 0, 1), 30.0f, 0.5f, 20.0f);

  // Perpendicular (theta = 90 deg)
  expect(spot.calculate_light_interaction(point3(5, 0, 0)), 0.0f);
  expect(spot.calculate_light_interaction(point3(0, 5, 0)), 0.0f);

  // Behind light (theta = 180 deg)
  expect(spot.calculate_light_interaction(point3(0, 0, -5)), 0.0f);

  // Obtuse angle (theta = 135 deg)
  expect(spot.calculate_light_interaction(point3(5, 0, -5)), 0.0f);
}

static void test_arbitrary_position_and_direction() {
  // Downward spotlight as placed in homework room scenes
  point3 pos(10, 20, 5);
  vec3 dir(0, -1, 0);
  spotlight spot(pos, dir, 35.0f, 0.5f, 20.0f);

  // On-axis 5 units straight down
  point3 p_on_axis(10, 15, 5);
  expect_near(spot.calculate_light_interaction(p_on_axis), 1.0f);

  // Opposite axis (straight up)
  point3 p_behind(10, 25, 5);
  expect(spot.calculate_light_interaction(p_behind), 0.0f);
}

int run_spotlight_tests(const char* filter) {
  int failed = 0;

  failed += run("test_initialization", test_initialization, filter);
  failed += run("test_smoothness_clamping", test_smoothness_clamping, filter);
  failed += run("test_direction_normalization", test_direction_normalization, filter);
  failed += run("test_on_axis_peak_intensity", test_on_axis_peak_intensity, filter);
  failed += run("test_inner_cone_linear_decay", test_inner_cone_linear_decay, filter);
  failed += run("test_inner_cone_monotonic_decrease", test_inner_cone_monotonic_decrease, filter);
  failed += run("test_outer_cone_exponential_decay", test_outer_cone_exponential_decay, filter);
  failed += run("test_outer_cone_boundary_cutoff", test_outer_cone_boundary_cutoff, filter);
  failed += run("test_sharp_boundary_when_smoothness_one", test_sharp_boundary_when_smoothness_one, filter);
  failed += run("test_zero_smoothness_immediate_outer_cone", test_zero_smoothness_immediate_outer_cone, filter);
  failed += run("test_distance_invariance", test_distance_invariance, filter);
  failed += run("test_radial_symmetry", test_radial_symmetry, filter);
  failed += run("test_perpendicular_and_behind", test_perpendicular_and_behind, filter);
  failed += run("test_arbitrary_position_and_direction", test_arbitrary_position_and_direction, filter);

  return failed;
}
