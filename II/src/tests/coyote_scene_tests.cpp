#include "test.h"
#include "instance.h"
#include "sphere.h"
#include "short_scenes.h"

static void test_moving_instance_scaled_interpolation() {
    material mat;
    sphere s(point3(0, 0, 0), 1.0f, &mat);

    vec3 trans0(10.0f, 0.0f, 0.0f);
    vec3 trans1(20.0f, 0.0f, 0.0f);
    float rot0 = 0.0f;
    float rot1 = 180.0f;
    vec3 scale(2.0f, 3.0f, 4.0f);
    vec3 base_rot(15.0f, 0.0f, 0.0f);

    moving_instance minst(&s, &mat, 0.0f, 1.0f, trans0, trans1, rot0, rot1, scale, base_rot);

    // Hit test ray through center at tau = 0.5 (expect sphere centered around x = 15.0)
    ray r(point3(15.0f, 0.0f, -10.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 1.0f, 0.5f);
    hit_record rec;
    expect_true(minst.hit(r, interval(0.001f, 100.0f), rec), "Ray should hit moving_instance at tau = 0.5");
}

static void test_coyote_scene_structure() {
    SceneData s = turtle_scene_coyote();

    expect_near(s.cam.lookfrom.x(), 0.0f);
    expect_near(s.cam.lookfrom.y(), 3.8f);
    expect_near(s.cam.lookfrom.z(), 2.0f);
    expect_near(s.cam.vfov, 48.0f);

    // Motion blur camera settings: t1 = 0.5f, fine csi (24 shutter instants)
    expect_near(s.cam.t0, 0.0f);
    expect_near(s.cam.t1, 0.5f);
    expect_near(s.cam.csi, 24.0f);
    expect((float)s.cam.samples_per_pixel, 24.0f);

    // Multi-frame animation: 3 seconds = 90 frames
    expect((float)s.camera_positions.length(), 90.0f);
    expect((float)s.frame_transforms.size(), 90.0f);

    // Frame 0: Coyote closer to camera (z = 8.0, x = 22), Ball at x = 1.7, Turtle upright (roll = 0, y = 0.35)
    auto& f0 = s.frame_transforms[0];
    expect_true(f0.size() >= 3, "Each frame must have transforms for Coyote, Ball, and Turtle");
    expect_near(f0[0].translationVector.x(), 22.0f);
    expect_near(f0[0].translationVector.z(), 8.0f); // Coyote closer to camera
    expect_near(f0[1].translationVector.x(), 1.7f);
    expect_near(f0[1].translationVector.y(), 0.60f);
    expect_true(f0[1].has_blur_override, "Ball must have motion blur enabled");
    expect_true(f0[1].blur_trans.x() > 0.1f, "Ball must have positive X motion blur streak");
    expect_near(f0[2].translationVector.y(), 0.35f);
    expect_near(f0[2].rotationVector.z(), 0.0f); // Upright at start

    // Immediate capsize when Coyote passes turtle (around frame 22-23)
    auto& f22 = s.frame_transforms[22];
    expect_true(f22[0].translationVector.x() <= 0.0f, "Coyote should reach or pass turtle by frame 22");
    expect_near(f22[2].rotationVector.z(), 90.0f); // Capsize snapping

    // Capsized and spinning on shell with motion blur (frame 24)
    auto& f24 = s.frame_transforms[24];
    expect_near(f24[2].translationVector.y(), 0.50f);
    expect_true(f24[2].rotationVector.z() > 175.0f, "Turtle should be rolled upside-down on shell");
    expect_true(f24[2].blur_rot_y > 100.0f, "Turtle should start spinning with high motion blur");

    // Deceleration mid-spin (frame 50)
    auto& f50 = s.frame_transforms[50];
    expect_true(f50[2].rotationVector.y() > 500.0f, "Turtle should have real frame spinning");
    expect_true(f50[2].blur_rot_y < f24[2].blur_rot_y, "Turtle motion blur should be reducing");

    // Finally stopped completely (frame 80)
    auto& f80 = s.frame_transforms[80];
    expect_near(f80[2].rotationVector.y(), 1800.0f);
    expect_near(f80[2].blur_rot_y, 0.0f); // Zero motion blur when stopped
    expect_near(f80[2].rotationVector.z(), 180.0f); // Stays upside down on shell

    // Final frame (frame 89, 3.0s mark): Coyote well past, Ball on right
    auto& f_end = s.frame_transforms[89];
    expect_true(f_end[0].translationVector.x() < -22.0f, "Coyote past screen on left");
    expect_near(f_end[1].translationVector.x(), 24.0f);
    expect_near(f_end[2].blur_rot_y, 0.0f);

    // World should contain ground plane, background rocks, cacti, cairn, ball, moving turtle, moving coyote (pole removed)
    expect_true(s.world.objects.size() >= 10, "World should contain scene objects");
    expect_true(s.allocated_hittables.size() == 1, "Only ball in allocated hittables, pole is removed");

    // Verify materials have toon shading enabled on models
    int toon_count = 0;
    for (auto mat : s.materials) {
        if (mat->is_toon) toon_count++;
    }
    expect_true(toon_count >= 5, "At least 5 materials should have toon shading enabled");

    // Apply frame 0 transforms and test initial ray hits
    for (auto& t : f0) {
        t.applied = false;
        t.apply();
    }
    // Coyote hit at z = 8.0 (rocket body passes around x = 21.25, y = 1.45 at tau = 0.25)
    bool coyote_hit = false;
    for (float dx = -2.0f; dx <= 2.0f; dx += 0.5f) {
        for (float dy = -1.0f; dy <= 1.0f; dy += 0.5f) {
            ray r_test(point3(21.25f + dx, 1.45f + dy, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.25f);
            hit_record rec_test;
            if (s.world.hit(r_test, interval(0.001f, 100.0f), rec_test)) {
                coyote_hit = true;
                break;
            }
        }
        if (coyote_hit) break;
    }
    expect_true(coyote_hit, "Ray should hit coyote at z = 8.0");

    // Ball hit with motion blur at tau = 0.25
    ray r_ball(point3(1.7f, 0.60f, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.25f);
    hit_record rec_ball;
    expect_true(s.world.hit(r_ball, interval(0.001f, 100.0f), rec_ball), "Ray should hit Pixar ball with motion blur");

    // Turtle hit at initial upright position
    ray r_turtle(point3(-0.5f, 0.35f, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.25f);
    hit_record rec_turtle;
    expect_true(s.world.hit(r_turtle, interval(0.001f, 100.0f), rec_turtle), "Ray should hit turtle at initial position");

    // Apply final frame transforms and test stopped capsized turtle hit
    for (auto& t : f_end) {
        t.applied = false;
        t.apply();
    }
    ray r_turtle_end(point3(-0.5f, 0.50f, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.25f);
    hit_record rec_turtle_end;
    expect_true(s.world.hit(r_turtle_end, interval(0.001f, 100.0f), rec_turtle_end), "Ray should hit stopped capsized turtle on shell");

    s.cleanup();
}

static void test_roadrunner_scene_structure() {
    SceneData s = turtle_scene_roadrunner();

    expect_near(s.cam.lookfrom.x(), 0.0f);
    expect_near(s.cam.lookfrom.y(), 3.8f);
    expect_near(s.cam.lookfrom.z(), 2.0f);
    expect_near(s.cam.vfov, 48.0f);

    // Motion blur camera settings: t1 = 0.5f, fine csi (24 shutter instants)
    expect_near(s.cam.t0, 0.0f);
    expect_near(s.cam.t1, 0.5f);
    expect_near(s.cam.csi, 24.0f);
    expect((float)s.cam.samples_per_pixel, 24.0f);

    // Multi-frame animation: 3.5 seconds = 105 frames
    expect((float)s.camera_positions.length(), 105.0f);
    expect((float)s.frame_transforms.size(), 105.0f);

    // Pole is removed
    expect_true(s.allocated_hittables.size() == 1, "Only ball in allocated hittables, pole is removed");

    // Frame 0:
    // Transforms: [0]=Roadrunner, [1]=Ball, [2]=Turtle, [3]=RockMid, [4]=RockTop, [5]=RockTip
    auto& f0 = s.frame_transforms[0];
    expect_true(f0.size() >= 6, "Each frame must have transforms for Roadrunner, Ball, Turtle, and 3 moving rocks");

    // Roadrunner off-screen right
    expect_near(f0[0].translationVector.x(), 24.0f);
    expect_near(f0[0].translationVector.z(), 8.0f);

    // Ball starts at x = 1.0 (waiting to be pushed by turtle)
    expect_near(f0[1].translationVector.x(), 1.0f);
    expect_near(f0[1].translationVector.y(), 0.60f);

    // Turtle starts at x = -1.2 walking slowly, upright
    expect_near(f0[2].translationVector.x(), -1.2f);
    expect_near(f0[2].translationVector.y(), 0.35f);
    expect_near(f0[2].rotationVector.y(), 0.0f);

    // Rocks scattered on the ground (y < 0.35)
    expect_true(f0[3].translationVector.y() < 0.35f, "Rock mid scattered on ground at start");
    expect_true(f0[4].translationVector.y() < 0.35f, "Rock top scattered on ground at start");
    expect_true(f0[5].translationVector.y() < 0.35f, "Rock tip scattered on ground at start");

    // Frame 80: Turtle and ball reached starting point for Coyote scene
    auto& f80 = s.frame_transforms[80];
    expect_near(f80[1].translationVector.x(), 1.7f); // Ball pushed to x = 1.7
    expect_near(f80[2].translationVector.x(), -0.5f); // Turtle walked to x = -0.5
    expect_near(f80[0].translationVector.x(), 24.0f); // Roadrunner still off-screen

    // Frame 104 (final frame):
    // Roadrunner is out of field on the left (x = -24.0)
    auto& f_end = s.frame_transforms[104];
    expect_near(f_end[0].translationVector.x(), -24.0f);
    expect_true(f_end[0].translationVector.x() <= -24.0f, "Roadrunner out of field at scene end");

    // Ball sitting at x = 1.7
    expect_near(f_end[1].translationVector.x(), 1.7f);

    // Turtle upright at x = -0.5, spinning with rotational motion blur = 60 deg (matching Coyote scene frame 0)
    expect_near(f_end[2].translationVector.x(), -0.5f);
    expect_near(f_end[2].translationVector.y(), 0.35f);
    expect_near(f_end[2].blur_rot_y, 60.0f);

    // Rocks stacked into the cairn tower (matching Coyote scene coordinates)
    expect_near(f_end[3].translationVector.x(), 3.15f);
    expect_near(f_end[3].translationVector.y(), 0.92f);
    expect_near(f_end[4].translationVector.x(), 3.10f);
    expect_near(f_end[4].translationVector.y(), 1.62f);
    expect_near(f_end[5].translationVector.x(), 3.08f);
    expect_near(f_end[5].translationVector.y(), 2.22f);

    // Toon shading on models
    int toon_count = 0;
    for (auto mat : s.materials) {
        if (mat->is_toon) toon_count++;
    }
    expect_true(toon_count >= 5, "At least 5 materials should have toon shading enabled");

    // Test ray hit on initial walking turtle (frame 0)
    for (auto& t : f0) {
        t.applied = false;
        t.apply();
    }
    ray r_turtle0(point3(-1.2f, 0.35f, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.0f);
    hit_record rec_turtle0;
    expect_true(s.world.hit(r_turtle0, interval(0.001f, 100.0f), rec_turtle0), "Ray should hit walking turtle at frame 0");

    // Test ray hit on final spinning turtle and stacked rocks (frame 104)
    for (auto& t : f_end) {
        t.applied = false;
        t.apply();
    }
    ray r_turtle_end(point3(-0.5f, 0.35f, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.0f);
    hit_record rec_turtle_end;
    expect_true(s.world.hit(r_turtle_end, interval(0.001f, 100.0f), rec_turtle_end), "Ray should hit spinning turtle at frame 104");

    ray r_cairn(point3(3.15f, 0.92f, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.0f);
    hit_record rec_cairn;
    expect_true(s.world.hit(r_cairn, interval(0.001f, 100.0f), rec_cairn), "Ray should hit stacked mid rock at frame 104");

    s.cleanup();
}

static void test_running_coyote_scene_structure() {
    SceneData s = running_coyote_scene();

    expect_near(s.cam.lookfrom.x(), 0.0f);
    expect_near(s.cam.lookfrom.y(), 3.8f);
    expect_near(s.cam.lookfrom.z(), 2.0f);
    expect_near(s.cam.vfov, 48.0f);

    expect_near(s.cam.t0, 0.0f);
    expect_near(s.cam.t1, 0.5f);
    expect_near(s.cam.csi, 48.0f);
    expect((float)s.cam.samples_per_pixel, 48.0f);

    expect((float)s.camera_positions.length(), 90.0f);
    expect((float)s.frame_transforms.size(), 90.0f);

    expect_true(s.world.objects.size() >= 35, "World should contain ground, distant rocks, midway formations, cacti, crow, foreground cacti, coyote, and dust cloud");

    // Frame transforms structure: 39 transforms per frame (5 distant rocks, 18 midway elements, 4 c1, 4 c2, 3 c3, 3 fg, coyote, dust)
    auto& f0 = s.frame_transforms[0];
    expect_true(f0.size() >= 35, "Each frame should have transforms for rocks, midway elements, cacti, crow, foreground cacti, coyote, and dust");

    // Background rocks drift along +X over 90 frames with Catmull-Rom spline with zero motion blur
    auto& f89 = s.frame_transforms[89];
    expect_true(f89[0].translationVector.x() > f0[0].translationVector.x(), "Background rock 1 should drift to the right");
    expect_true(!f0[0].has_blur_override, "Background rocks must have absolutely no motion blur");

    // Midway elements drift along +X with optical parallax and zero motion blur
    expect_true(f89[5].translationVector.x() > f0[5].translationVector.x(), "Midway mesa should drift to the right");
    expect_true(!f0[5].has_blur_override, "Midway elements must have no motion blur");
    expect_true(f89[6].translationVector.x() > f0[6].translationVector.x(), "Midway spire should drift to the right");
    expect_true(!f0[6].has_blur_override, "Midway elements must have no motion blur");

    // Group 1 cacti + accent rock crossing at frame 15 with moderate speed and low motion blur (moving left to right)
    auto& f15 = s.frame_transforms[15];
    expect_true(f15[23].has_blur_override, "Group 1 cactus should have motion blur enabled during crossing");
    expect_true(f15[23].blur_trans.x() >= 0.4f && f15[23].blur_trans.x() <= 0.8f, "Group 1 cactus should have significantly reduced motion blur");
    expect_true(f15[23].translationVector.x() > -20.0f && f15[23].translationVector.x() < 20.0f, "Group 1 cactus on-screen at frame 15");

    // Group 2 cacti with perched Crow crossing at frame 46 in lockstep
    auto& f46 = s.frame_transforms[46];
    expect_true(f46[27].has_blur_override, "Group 2 hero cactus has blur");
    expect_true(f46[28].has_blur_override, "Crow has blur override");
    expect_true(f46[28].blur_trans.x() >= 0.4f && f46[28].blur_trans.x() <= 0.8f, "Crow has reduced blur matching cactus");
    expect_near(f46[28].translationVector.y(), 2.706f); // Perched on cactus branch
    expect_near(f46[28].translationVector.x(), f46[27].translationVector.x() + 0.018f); // Moving in lockstep with cactus
    expect_true(f46[27].translationVector.x() > -20.0f && f46[27].translationVector.x() < 20.0f, "Group 2 hero cactus on-screen at frame 46");

    // Group 3 cacti crossing at frame 75 with moderate speed and low motion blur
    auto& f75 = s.frame_transforms[75];
    expect_true(f75[31].has_blur_override, "Group 3 cactus should have motion blur enabled during crossing");
    expect_true(f75[31].blur_trans.x() >= 0.4f && f75[31].blur_trans.x() <= 0.8f, "Group 3 cactus should have reduced motion blur");
    expect_true(f75[31].translationVector.x() > -20.0f && f75[31].translationVector.x() < 20.0f, "Group 3 cactus on-screen at frame 75");

    // Foreground blazing fast cacti directly obstructing camera view at frame 40
    auto& f40 = s.frame_transforms[40];
    expect_true(f40[34].has_blur_override, "Foreground cactus must have motion blur override enabled");
    expect_true(f40[34].blur_trans.x() >= 5.0f, "Foreground cactus must have highest motion blur streak");
    expect_true(f40[34].translationVector.x() > -3.0f && f40[34].translationVector.x() < 3.0f, "Foreground cactus directly obstructs camera view at frame 40");
    expect_true(f40[34].translationVector.z() <= 6.0f, "Foreground cactus must be close in foreground");

    // Dust cloud trailing behind Coyote on left side of screen (screen left = +X in camera coordinates)
    expect_true(f0[38].translationVector.x() > 3.0f, "Dust cloud must be on left side of screen (+X in camera coords)");
    expect_true(f0[38].has_blur_override, "Dust cloud must have motion blur override");
    expect_true(f0[38].blur_trans.x() > 0.0f, "Dust cloud motion blur must trail backwards to the left (+X)");

    bool coyote_hit_end = false;
    for (float dx = -2.0f; dx <= 2.0f; dx += 0.5f) {
        for (float dy = -1.0f; dy <= 1.0f; dy += 0.5f) {
            ray r_test(point3(0.0f + dx, 1.85f + dy, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.50f);
            hit_record rec_test;
            if (s.world.hit(r_test, interval(0.001f, 100.0f), rec_test)) {
                coyote_hit_end = true;
                break;
            }
        }
        if (coyote_hit_end) break;
    }
    expect_true(coyote_hit_end, "Ray should hit coyote at center at tau = 0.50 in running_coyote_scene");

    bool coyote_hit_mid = false;
    for (float dx = -2.0f; dx <= 2.0f; dx += 0.5f) {
        for (float dy = -1.0f; dy <= 1.0f; dy += 0.5f) {
            ray r_test(point3(0.0f + dx, 1.85f + dy, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.25f);
            hit_record rec_test;
            if (s.world.hit(r_test, interval(0.001f, 100.0f), rec_test)) {
                coyote_hit_mid = true;
                break;
            }
        }
        if (coyote_hit_mid) break;
    }
    expect_true(coyote_hit_mid, "Ray should hit coyote at center at tau = 0.25 in running_coyote_scene");

    bool dust_hit = false;
    for (float dx = -2.0f; dx <= 2.0f; dx += 0.5f) {
        for (float dy = -0.5f; dy <= 0.5f; dy += 0.25f) {
            ray r_test(point3(4.8f + dx, 0.95f + dy, 2.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.25f);
            hit_record rec_test;
            if (s.world.hit(r_test, interval(0.001f, 100.0f), rec_test)) {
                dust_hit = true;
                break;
            }
        }
        if (dust_hit) break;
    }
    expect_true(dust_hit, "Ray should hit motion blurred dust cloud on left side in running_coyote_scene");

    s.cleanup();
}

static void test_rolling_tumbleweed_scene_structure() {
    SceneData s = rolling_tumbleweed_scene();

    expect_near(s.cam.lookfrom.x(), 0.0f);
    expect_near(s.cam.lookfrom.y(), 5.0f);
    expect_near(s.cam.lookfrom.z(), 0.0f);
    expect_near(s.cam.vfov, 50.0f);

    // Camera shutter motion blur settings
    expect_near(s.cam.t0, 0.0f);
    expect_near(s.cam.t1, 0.5f);
    expect_near(s.cam.csi, 48.0f);
    expect((float)s.cam.samples_per_pixel, 48.0f);

    // Duration: 5 seconds = 150 frames
    expect((float)s.camera_positions.length(), 150.0f);
    expect((float)s.frame_transforms.size(), 150.0f);

    // Frame 0: transforms for Tumbleweed + 3 dust clouds (bg, mg, near)
    auto& f0 = s.frame_transforms[0];
    expect_true(f0.size() >= 4, "Each frame must have transforms for Tumbleweed and 3 dust clouds");

    // Tumbleweed start at outer left
    expect_near(f0[0].translationVector.x(), 16.0f);
    expect_true(f0[0].has_blur_override, "Tumbleweed must have motion blur enabled");
    expect_near(f0[0].blur_trans.x(), -0.28f);

    // Depth layers and blur streaks
    expect_near(f0[1].translationVector.z(), 55.0f); // Background
    expect_near(f0[1].blur_trans.x(), -0.12f);

    expect_near(f0[2].translationVector.z(), 28.0f); // Midground
    expect_near(f0[2].blur_trans.x(), -0.20f);

    expect_near(f0[3].translationVector.z(), 16.0f); // Near puff
    expect_near(f0[3].blur_trans.x(), -0.28f);

    // Parallax speed check: apparent angular speed across screen (swath / depth)
    // Closer objects must have strictly greater apparent speed and blur streak than further objects
    auto& f_end = s.frame_transforms[149];
    float app_speed_bg = fabs(f_end[1].translationVector.x() - f0[1].translationVector.x()) / f0[1].translationVector.z();
    float app_speed_mg = fabs(f_end[2].translationVector.x() - f0[2].translationVector.x()) / f0[2].translationVector.z();
    float app_speed_near = fabs(f_end[3].translationVector.x() - f0[3].translationVector.x()) / f0[3].translationVector.z();

    expect_true(app_speed_near > app_speed_mg, "Near apparent speed > Midground apparent speed");
    expect_true(app_speed_mg > app_speed_bg, "Midground apparent speed > Background apparent speed");

    // Motion blur magnitude must scale with speed/proximity
    expect_true(fabs(f0[3].blur_trans.x()) > fabs(f0[2].blur_trans.x()), "Near blur streak > MG blur streak");
    expect_true(fabs(f0[2].blur_trans.x()) > fabs(f0[1].blur_trans.x()), "MG blur streak > BG blur streak");

    // Ray hit test on tumbleweed at frame 0
    for (auto& t : f0) {
        t.applied = false;
        t.apply();
    }
    ray r_tw(point3(16.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.5f, 0.0f);
    hit_record rec_tw;
    expect_true(s.world.hit(r_tw, interval(0.001f, 100.0f), rec_tw), "Ray should hit tumbleweed at frame 0");

    s.cleanup();
}

static void test_coyote_time_scene_structure() {
    SceneData s = coyote_time_scene();

    expect_near(s.cam.lookfrom.x(), 0.0f);
    expect_near(s.cam.lookfrom.y(), 3.8f);
    expect_near(s.cam.lookfrom.z(), 2.0f);
    expect_near(s.cam.vfov, 48.0f);

    // Shutter motion blur camera settings
    expect_near(s.cam.t0, 0.0f);
    expect_near(s.cam.t1, 0.5f);
    expect_near(s.cam.csi, 24.0f);
    expect((float)s.cam.samples_per_pixel, 24.0f);

    // Duration: 10 seconds = 300 frames
    expect((float)s.camera_positions.length(), 300.0f);
    expect((float)s.frame_transforms.size(), 300.0f);

    // Frame 0: Roadrunner rushes in from x = -18, Coyote offscreen at x = -26
    auto& f0 = s.frame_transforms[0];
    expect_true(f0.size() >= 2, "Each frame must have transforms for Roadrunner and Coyote");
    expect_near(f0[0].translationVector.x(), -18.0f);
    expect_true(f0[0].blur_trans.x() > 1.0f, "Roadrunner must have forward motion blur at frame 0");
    expect_near(f0[1].translationVector.x(), -26.0f);

    // Frame 35: Roadrunner stopped at cliff edge (x = -0.8f) with zero blur
    auto& f35 = s.frame_transforms[35];
    expect_near(f35[0].translationVector.x(), -0.8f);
    expect_near(f35[0].blur_trans.x(), 0.0f);

    // Frame 90: Coyote has passed the edge (x >= 0.0f) and camera is tracking
    auto& f90 = s.frame_transforms[90];
    expect_true(f90[1].translationVector.x() >= 0.0f, "Coyote should pass edge by frame 90");
    expect_true(s.camera_positions.points[90].x() > 0.0f, "Camera should be tracking Coyote");

    // Frame 130 to 219: 90 frames (exactly 3.0s) pause in mid-air over the abyss
    auto& f130 = s.frame_transforms[130];
    expect_near(f130[1].translationVector.x(), 5.5f);
    expect_near(f130[1].translationVector.y(), 1.85f);
    expect_near(f130[1].blur_trans.x(), 0.0f);
    expect_near(f130[1].blur_trans.y(), 0.0f);
    expect_near(s.camera_positions.points[130].x(), 3.5f);

    auto& f219 = s.frame_transforms[219];
    expect_near(f219[1].translationVector.x(), 5.5f);
    expect_near(f219[1].translationVector.y(), 1.85f);
    expect_near(f219[1].blur_trans.x(), 0.0f);
    expect_near(f219[1].blur_trans.y(), 0.0f);
    expect_near(s.camera_positions.points[219].x(), 3.5f);

    // Frame 230: Coyote falling with vertical downward motion blur
    auto& f230 = s.frame_transforms[230];
    expect_true(f230[1].translationVector.y() < 1.85f, "Coyote should drop along -Y");
    expect_true(f230[1].blur_trans.y() < 0.0f, "Coyote should have downward vertical motion blur streak");

    // Frame 260+: Coyote out of field at the bottom (y < -20.0f)
    auto& f260 = s.frame_transforms[260];
    expect_true(f260[1].translationVector.y() < -20.0f, "Coyote should plunge out of field at bottom");

    // World contains plateau quad, cliff wall, canyon floor, rocks, cacti, roadrunner, coyote
    expect_true(s.world.objects.size() >= 10, "World should contain scene objects");

    s.cleanup();
}

static void test_final_falling_coyote_scene_structure() {
    SceneData s = final_falling_coyote_scene();

    // Top-down camera pointing along -Y
    expect_near(s.cam.lookfrom.x(), 0.0f);
    expect_near(s.cam.lookfrom.y(), 26.0f);
    expect_near(s.cam.lookfrom.z(), 0.0f);
    expect_near(s.cam.lookat.y(), 0.0f);
    expect_near(s.cam.vup.z(), -1.0f);
    expect_near(s.cam.vfov, 52.0f);

    // No motion blur camera settings
    expect_near(s.cam.t0, 0.0f);
    expect_near(s.cam.t1, 0.0f);
    expect((float)s.cam.samples_per_pixel, 1.0f);

    // Duration: 3.5 seconds = 105 frames
    expect((float)s.camera_positions.length(), 105.0f);
    expect((float)s.frame_transforms.size(), 105.0f);

    // World contains canyon floor quad, fallback plane, canyon wall rocks (2 left, 2 right), and Coyote
    expect_true(s.world.objects.size() >= 6, "World should contain floor and canyon walls and coyote");

    // Frame 0: Coyote near camera at y = 22.0f, no motion blur, exhaust facing camera (pitch ~ 90)
    auto& f0 = s.frame_transforms[0];
    expect_true(f0.size() >= 1, "Each frame must have Coyote transform");
    expect_near(f0[0].translationVector.y(), 22.0f);
    expect_true(f0[0].rotationVector.x() > 85.0f, "Rocket pitch should be ~90 deg with exhaust facing camera");
    expect_true(!f0[0].has_blur_override, "No motion blur override on Coyote");
    expect_near(f0[0].blur_trans.y(), 0.0f);

    // Frame 52 (approx midway): quadratic falling y = 22 - 60.5 * 0.25 = ~6.875
    auto& f52 = s.frame_transforms[52];
    expect_true(f52[0].translationVector.y() < 10.0f, "Coyote should drop quadratically");
    expect_true(f52[0].translationVector.y() > 0.0f, "Coyote midway should still be above 0");

    // Frame 104 (final frame): Coyote at the valley bottom (y <= -38.0f)
    auto& f104 = s.frame_transforms[104];
    expect_true(f104[0].translationVector.y() <= -38.0f, "Coyote should reach valley bottom");
    expect_true(f104[0].scaleVector.x() < 0.035f, "Coyote scale should decrease quadratically");

    s.cleanup();
}

int run_coyote_scene_tests(const char* filter) {
    int failed = 0;
    failed += run("test_moving_instance_scaled_interpolation", test_moving_instance_scaled_interpolation, filter);
    failed += run("test_roadrunner_scene_structure", test_roadrunner_scene_structure, filter);
    failed += run("test_coyote_scene_structure", test_coyote_scene_structure, filter);
    failed += run("test_running_coyote_scene_structure", test_running_coyote_scene_structure, filter);
    failed += run("test_rolling_tumbleweed_scene_structure", test_rolling_tumbleweed_scene_structure, filter);
    failed += run("test_coyote_time_scene_structure", test_coyote_time_scene_structure, filter);
    failed += run("test_final_falling_coyote_scene_structure", test_final_falling_coyote_scene_structure, filter);
    return failed;
}
