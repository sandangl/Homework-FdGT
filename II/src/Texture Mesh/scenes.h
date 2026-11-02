#pragma once

#include <list>
#include <memory>
#include <iostream>

#include "vec3.h"
#include "color.h"
#include "camera.h"
#include "hittable_list.h"
#include "light_list.h"
#include "light.h"
#include "ext_light.h"
#include "material.h"
#include "texture.h"
#include "sphere.h"
#include "geometry.h"
#include "instance.h"

#if USE_TINYBVH
#include "mesh.h"
#else
#include "mesh_bruteforce.h"
#endif

using std::make_shared;

struct SceneData {
    hittable_list world;
    light_list lights;
    camera cam;

    std::list<mesh*> meshes;
    std::list<texture*> textures;
    std::list<material*> materials;
    std::list<light*> allocated_lights;
    std::list<ext_light*> allocated_ext_lights;
    std::list<hittable*> allocated_hittables;

    void cleanup() {
        for (auto m : meshes) delete m;
        for (auto t : textures) delete t;
        for (auto mat : materials) delete mat;
        for (auto l : allocated_lights) delete l;
        for (auto el : allocated_ext_lights) delete el;
        for (auto h : allocated_hittables) delete h;

        meshes.clear();
        textures.clear();
        materials.clear();
        allocated_lights.clear();
        allocated_ext_lights.clear();
        allocated_hittables.clear();
        world.clear();
        lights.clear();
    }
};

inline void cleanup_scene(SceneData& scene) {
    scene.cleanup();
}

// Internal helper for scenes 1-5 (Bunny room)
inline void setup_bunny_room(SceneData& s, bool include_rear_wall, bool include_obstruction) {
    color lightgray(0.75f, 0.75f, 0.75f);
    color faintred(1.0f, 0.55f, 0.55f);
    color faintblue(0.55f, 0.55f, 1.0f);

    // Camera setup for bunny room
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 700;
    s.cam.samples_per_pixel = 1;
    s.cam.vfov = 20;
    s.cam.lookfrom = point3(0.0f, 50.0f, 30.0f);
    s.cam.lookat = point3(0.0f, -30.0f, -120.0f);

    // Bunny model
    mesh* bunny = new mesh("../models/bunny.obj", "../models/");
    if (!bunny->loaded) {
        std::cerr << "Impossibile caricare ../models/bunny.obj" << std::endl;
    }
    s.meshes.push_back(bunny);

    material* bunny_mat = new material();
    bunny_mat->ka = lightgray;
    bunny_mat->kd = lightgray;
    bunny_mat->ks = lightgray;
    s.materials.push_back(bunny_mat);

    auto bunny_inst = make_shared<instance>(bunny, bunny_mat);
    bunny_inst->scale(10.0f, 10.0f, 10.0f);
    bunny_inst->rotate_y(8.0f);
    bunny_inst->translate(0.0f, -40.0f, -120.0f);
    s.world.add(bunny_inst);

    // Spheres
    material* red_mat = new material();
    red_mat->ka = faintred;
    red_mat->kd = faintred;
    red_mat->ks = faintred;
    s.materials.push_back(red_mat);

    material* blue_mat = new material();
    blue_mat->ka = faintblue;
    blue_mat->kd = faintblue;
    blue_mat->ks = faintblue;
    s.materials.push_back(blue_mat);

    auto red_sphere = make_shared<sphere>(point3(-27.0f, -30.0f, -118.0f), 10.0f, red_mat);
    auto blue_sphere = make_shared<sphere>(point3(23.0f, -32.0f, -115.0f), 8.0f, blue_mat);
    s.world.add(red_sphere);
    s.world.add(blue_sphere);

    // Floor and Wall Texture
    texture* room_tex = new image_texture("../models/texturecheckerboard.png");
    s.textures.push_back(room_tex);

    material* floor_mat = new material();
    floor_mat->tex = room_tex;
    floor_mat->ka = color(0.2f, 0.2f, 0.2f);
    floor_mat->kd = color(0.5f, 0.5f, 0.5f);
    floor_mat->ks = color(0.1f, 0.1f, 0.1f);
    floor_mat->alpha = 10.0f;
    s.materials.push_back(floor_mat);

    auto floor_plane = make_shared<plane>(point3(0.0f, -40.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), floor_mat);
    s.world.add(floor_plane);

    if (include_rear_wall) {
        material* wall_mat = new material();
        wall_mat->tex = room_tex;
        wall_mat->ka = color(0.2f, 0.2f, 0.22f);
        wall_mat->kd = color(0.6f, 0.6f, 0.65f);
        wall_mat->ks = color(0.1f, 0.1f, 0.1f);
        wall_mat->alpha = 10.0f;
        s.materials.push_back(wall_mat);

        auto back_wall = make_shared<plane>(point3(0.0f, 0.0f, -128.0f), vec3(0.0f, 0.0f, 1.0f), wall_mat);
        s.world.add(back_wall);
    }

    if (include_obstruction) {
        material* obs_mat = new material();
        obs_mat->ka = color(0.15f, 0.15f, 0.15f);
        obs_mat->kd = color(0.85f, 0.65f, 0.2f);
        obs_mat->ks = color(0.5f, 0.5f, 0.5f);
        obs_mat->alpha = 40.0f;
        s.materials.push_back(obs_mat);

        auto obstruction = make_shared<sphere>(point3(0.0f, -2.0f, -115.0f), 6.0f, obs_mat);
        s.world.add(obstruction);
    }
}

// Scene 1: This scene without obstruction sphere, spotlight only
inline SceneData create_scene_1_spotlight_only() {
    SceneData s;
    setup_bunny_room(s, true, false);

    point3 spotlight_position(30.0f, 60.0f, -60.0f);
    vec3 spotlight_direction = point3(0.0f, -40.0f, -120.0f) - spotlight_position;
    spotlight* spot = new spotlight(spotlight_position, spotlight_direction, 20.0f, 1.0f,
                                    color(0.05f, 0.05f, 0.05f), color(0.9f, 0.9f, 0.9f), color(0.5f, 0.5f, 0.5f));
    s.lights.add(spot);
    s.allocated_lights.push_back(spot);

    return s;
}

// Scene 2: This scene without rear wall and obstruction sphere, all light sources enabled
inline SceneData create_scene_2_no_wall_all_lights() {
    SceneData s;
    setup_bunny_room(s, false, false);

    color black(0.0f, 0.0f, 0.0f);
    color lightgray(0.75f, 0.75f, 0.75f);

    // Point light
    point_light* worldlight = new point_light(point3(6.0f, 6.0f, 0.0f), black, lightgray, black);
    s.lights.add(worldlight);
    s.allocated_lights.push_back(worldlight);

    // Directional light
    directional_light* dirlight = new directional_light(vec3(0.5f, 1.0f, 0.5f), black, lightgray, lightgray);
    s.lights.add(dirlight);
    s.allocated_lights.push_back(dirlight);

    // Spotlight
    point3 spotlight_position(30.0f, 60.0f, -60.0f);
    vec3 spotlight_direction = point3(0.0f, -40.0f, -120.0f) - spotlight_position;
    spotlight* spot = new spotlight(spotlight_position, spotlight_direction, 20.0f, 1.0f,
                                    color(0.05f, 0.05f, 0.05f), color(0.9f, 0.9f, 0.9f), color(0.5f, 0.5f, 0.5f));
    s.lights.add(spot);
    s.allocated_lights.push_back(spot);

    // Extended light
    point3 Q(-30.0f, 40.0f, -130.0f);
    vec3 u(60.0f, 0.0f, 0.0f);
    vec3 v(0.0f, 0.0f, 40.0f);
    ext_light* extended = new point_ext_light(Q, u, v, black, lightgray, black, 256);
    light_list ext_sources = extended->getSources();
    s.lights.add(ext_sources);
    for (auto l : ext_sources.sources) s.allocated_lights.push_back(l);
    s.allocated_ext_lights.push_back(extended);

    return s;
}

// Scene 3: This scene without obstruction sphere, 3 spotlights (left, right, center) pointing at the same point3
inline SceneData create_scene_3_three_spotlights() {
    SceneData s;
    setup_bunny_room(s, true, false);

    point3 target(0.0f, -30.0f, -120.0f);

    // Center spotlight (white)
    point3 pos_center(0.0f, 60.0f, -60.0f);
    spotlight* spot_c = new spotlight(pos_center, target - pos_center, 25.0f, 0.5f,
                                      color(0.05f, 0.05f, 0.05f), color(0.85f, 0.85f, 0.85f), color(0.5f, 0.5f, 0.5f));
    s.lights.add(spot_c);
    s.allocated_lights.push_back(spot_c);

    // Left spotlight (red-tinted)
    point3 pos_left(-45.0f, 50.0f, -60.0f);
    spotlight* spot_l = new spotlight(pos_left, target - pos_left, 25.0f, 0.5f,
                                      color(0.05f, 0.02f, 0.02f), color(0.9f, 0.35f, 0.35f), color(0.5f, 0.5f, 0.5f));
    s.lights.add(spot_l);
    s.allocated_lights.push_back(spot_l);

    // Right spotlight (blue-tinted)
    point3 pos_right(45.0f, 50.0f, -60.0f);
    spotlight* spot_r = new spotlight(pos_right, target - pos_right, 25.0f, 0.5f,
                                      color(0.02f, 0.02f, 0.05f), color(0.35f, 0.55f, 0.95f), color(0.5f, 0.5f, 0.5f));
    s.lights.add(spot_r);
    s.allocated_lights.push_back(spot_r);

    return s;
}

// Scene 4: This very scene, only with extended light (with obstruction)
inline SceneData create_scene_4_extended_light_only() {
    SceneData s;
    setup_bunny_room(s, true, true);

    color black(0.0f, 0.0f, 0.0f);
    color lightgray(0.75f, 0.75f, 0.75f);

    point3 Q(-30.0f, 40.0f, -130.0f);
    vec3 u(60.0f, 0.0f, 0.0f);
    vec3 v(0.0f, 0.0f, 40.0f);
    ext_light* extended = new point_ext_light(Q, u, v, black, lightgray, black, 256);
    light_list ext_sources = extended->getSources();
    s.lights.add(ext_sources);
    for (auto l : ext_sources.sources) s.allocated_lights.push_back(l);
    s.allocated_ext_lights.push_back(extended);

    return s;
}

// Scene 5: This very scene, all lights enabled (with obstruction)
inline SceneData create_scene_5_all_lights() {
    SceneData s;
    setup_bunny_room(s, true, true);

    color black(0.0f, 0.0f, 0.0f);
    color lightgray(0.75f, 0.75f, 0.75f);

    // Point light
    point_light* worldlight = new point_light(point3(6.0f, 6.0f, 0.0f), black, lightgray, black);
    s.lights.add(worldlight);
    s.allocated_lights.push_back(worldlight);

    // Directional light
    directional_light* dirlight = new directional_light(vec3(0.5f, 1.0f, 0.5f), black, lightgray, lightgray);
    s.lights.add(dirlight);
    s.allocated_lights.push_back(dirlight);

    // Spotlight
    point3 spotlight_position(30.0f, 60.0f, -60.0f);
    vec3 spotlight_direction = point3(0.0f, -40.0f, -120.0f) - spotlight_position;
    spotlight* spot = new spotlight(spotlight_position, spotlight_direction, 20.0f, 1.0f,
                                    color(0.05f, 0.05f, 0.05f), color(0.9f, 0.9f, 0.9f), color(0.5f, 0.5f, 0.5f));
    s.lights.add(spot);
    s.allocated_lights.push_back(spot);

    // Extended light
    point3 Q(-30.0f, 40.0f, -130.0f);
    vec3 u(60.0f, 0.0f, 0.0f);
    vec3 v(0.0f, 0.0f, 40.0f);
    ext_light* extended = new point_ext_light(Q, u, v, black, lightgray, black, 256);
    light_list ext_sources = extended->getSources();
    s.lights.add(ext_sources);
    for (auto l : ext_sources.sources) s.allocated_lights.push_back(l);
    s.allocated_ext_lights.push_back(extended);

    return s;
}

// Scene 6: Original main state (Cat mesh with texturecat.jpg, point light, original camera)
inline SceneData create_scene_6_original_cat() {
    SceneData s;
    color lightgray(0.75f, 0.75f, 0.75f);
    color black(0.0f, 0.0f, 0.0f);

    // Original camera
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 700;
    s.cam.samples_per_pixel = 1;
    s.cam.vfov = 20;
    s.cam.lookfrom = point3(0, 2, 10);
    s.cam.lookat = point3(0, 0, -1);

    // Original point light
    point_light* worldlight = new point_light(point3(6.0f, 6.0f, 0.0f), black, lightgray, black);
    s.lights.add(worldlight);
    s.allocated_lights.push_back(worldlight);

    // Cat mesh and texture
    mesh* cat = new mesh("../models/cat.obj", "../models/");
    if (!cat->loaded) {
        std::cerr << "Impossibile caricare ../models/cat.obj" << std::endl;
    }
    s.meshes.push_back(cat);

    texture* cat_tex = new image_texture("../models/texturecat.jpg");
    s.textures.push_back(cat_tex);

    material* cat_mat = new material();
    cat_mat->tex = cat_tex;
    cat_mat->ka = lightgray;
    cat_mat->ks = lightgray;
    s.materials.push_back(cat_mat);

    auto cat_inst = make_shared<instance>(cat, cat_mat);
    cat_inst->scale(1.0f, 1.0f, 1.0f);
    cat_inst->rotate_y(90.0f);
    cat_inst->rotate_z(90.0f);
    cat_inst->translate(0.0f, -40.0f, -120.0f);
    s.world.add(cat_inst);

    return s;
}

inline SceneData create_toon_scene() {
    SceneData s;
    color lightgray(0.75f, 0.75f, 0.75f);
    color faintred(1.0f, 0.55f, 0.55f);
    color faintblue(0.55f, 0.55f, 1.0f);

    // Camera setup for toon scene matching slide 4
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 700;
    s.cam.samples_per_pixel = 1;
    s.cam.vfov = 20;
    s.cam.lookfrom = point3(0.0f, 50.0f, 30.0f);
    s.cam.lookat = point3(0.0f, -30.0f, -120.0f);

    // Directional light (sun) from front-left-above casting shadows backwards-right
    directional_light* sun = new directional_light(
        vec3(-0.31f, 0.33f, 0.89f),
        color(0.05f, 0.05f, 0.05f),
        color(0.85f, 0.85f, 0.85f),
        color(0.85f, 0.85f, 0.85f)
    );
    s.lights.add(sun);
    s.allocated_lights.push_back(sun);

    // Center object: Stanford Bunny
    mesh* bunny = new mesh("../models/bunny.obj", "../models/");
    if (!bunny->loaded) {
        std::cerr << "Impossibile caricare ../models/bunny.obj" << std::endl;
    }
    s.meshes.push_back(bunny);

    material* bunny_mat = new material();
    bunny_mat->ka = lightgray;
    bunny_mat->kd = lightgray;
    bunny_mat->ks = lightgray;
    bunny_mat->is_toon = true;
    s.materials.push_back(bunny_mat);

    auto bunny_inst = make_shared<instance>(bunny, bunny_mat);
    bunny_inst->scale(10.0f, 10.0f, 10.0f);
    bunny_inst->rotate_y(8.0f);
    bunny_inst->translate(0.0f, -40.0f, -120.0f);
    s.world.add(bunny_inst);

    // Left object: Red sphere
    material* red_mat = new material();
    red_mat->ka = faintred;
    red_mat->kd = faintred;
    red_mat->ks = faintred;
    red_mat->is_toon = true;
    s.materials.push_back(red_mat);

    auto red_sphere = make_shared<sphere>(point3(-27.0f, -30.0f, -118.0f), 10.0f, red_mat);
    s.world.add(red_sphere);

    // Right object: Blue sphere
    material* blue_mat = new material();
    blue_mat->ka = faintblue;
    blue_mat->kd = faintblue;
    blue_mat->ks = faintblue;
    blue_mat->is_toon = true;
    s.materials.push_back(blue_mat);

    auto blue_sphere = make_shared<sphere>(point3(23.0f, -32.0f, -115.0f), 8.0f, blue_mat);
    s.world.add(blue_sphere);

    // Floor plane: uniform light gray without checkerboard, no back wall
    material* floor_mat = new material();
    floor_mat->ka = color(0.2f, 0.2f, 0.2f);
    floor_mat->kd = color(0.7f, 0.7f, 0.7f);
    floor_mat->ks = color(0.1f, 0.1f, 0.1f);
    floor_mat->alpha = 10.0f;
    floor_mat->is_toon = false; // Turn off toon on floor to avoid dark horizon
    s.materials.push_back(floor_mat);

    auto floor_plane = make_shared<plane>(point3(0.0f, -40.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), floor_mat);
    s.world.add(floor_plane);

    return s;
}

// Common objects for slide 9 scenes: Stanford Bunny, red sphere, blue sphere, curved ground sphere
inline void setup_slide9_objects(SceneData& s) {
    color lightgray(0.75f, 0.75f, 0.75f);
    color faintred(1.0f, 0.55f, 0.55f);
    color faintblue(0.55f, 0.55f, 1.0f);

    // Center object: Stanford Bunny
    mesh* bunny = new mesh("../models/bunny.obj", "../models/");
    if (!bunny->loaded) {
        std::cerr << "Impossibile caricare ../models/bunny.obj" << std::endl;
    }
    s.meshes.push_back(bunny);

    material* bunny_mat = new material();
    bunny_mat->ka = lightgray;
    bunny_mat->kd = lightgray;
    bunny_mat->ks = lightgray;
    s.materials.push_back(bunny_mat);

    auto bunny_inst = make_shared<instance>(bunny, bunny_mat);
    bunny_inst->scale(10.0f, 10.0f, 10.0f);
    bunny_inst->rotate_y(8.0f);
    bunny_inst->translate(0.0f, -48.1f, -120.0f);
    s.world.add(bunny_inst);

    // Left object: Red sphere (tangent to ground sphere at -48.6)
    material* red_mat = new material();
    red_mat->ka = faintred;
    red_mat->kd = faintred;
    red_mat->ks = faintred;
    s.materials.push_back(red_mat);

    auto red_sphere = make_shared<sphere>(point3(-27.0f, -38.4f, -118.0f), 10.0f, red_mat);
    s.world.add(red_sphere);

    // Right object: Blue sphere (tangent to ground sphere at -47.8)
    material* blue_mat = new material();
    blue_mat->ka = faintblue;
    blue_mat->kd = faintblue;
    blue_mat->ks = faintblue;
    s.materials.push_back(blue_mat);

    auto blue_sphere = make_shared<sphere>(point3(23.0f, -39.6f, -115.0f), 8.0f, blue_mat);
    s.world.add(blue_sphere);

    // Curved ground: large sphere creating the curved horizon
    material* ground_mat = new material();
    ground_mat->ka = color(0.2f, 0.2f, 0.2f);
    ground_mat->kd = color(0.7f, 0.7f, 0.7f);
    ground_mat->ks = color(0.1f, 0.1f, 0.1f);
    ground_mat->alpha = 10.0f;
    s.materials.push_back(ground_mat);

    auto ground_sphere = make_shared<sphere>(point3(0.0f, -440.0f, -40.0f), 400.0f, ground_mat);
    s.world.add(ground_sphere);
}

// Scene 1: Sky equirectangular texture with directional light from estimated sun
inline SceneData create_sky_scene() {
    SceneData s;
    setup_slide9_objects(s);

    color lightgray(0.75f, 0.75f, 0.75f);
    color ambient_light(0.25f, 0.25f, 0.25f);

    // Camera setup
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 700;
    s.cam.samples_per_pixel = 1;
    s.cam.vfov = 20;
    s.cam.lookfrom = point3(0.0f, 50.0f, 30.0f);
    s.cam.lookat = point3(0.0f, -30.0f, -120.0f);

    // Sky texture: prefers upscaled 2048x1024 texture if present
    const char* sky_path = "../models/canyon_sky.png";
    FILE* test_f = fopen(sky_path, "rb");
    if (test_f) { fclose(test_f); } else { sky_path = "../models/canyon_sky_small.png"; }

    environment_texture* sky_tex = new environment_texture(sky_path);
    s.cam.environment_texture_sky = sky_tex;
    s.textures.push_back(sky_tex);

    // Directional light from estimated sun direction
    directional_light* dir_light = new directional_light(sky_tex->sun_direction, ambient_light, lightgray, lightgray);
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);

    // Print sun analysis
    float true_x = 0.0f, true_y = 0.0f;
    vec3 true_sun_dir = sky_tex->compute_ground_truth_sun(true_x, true_y);
    float err_deg = sky_tex->angular_error_degrees(true_sun_dir);
    std::cout << "Sky Texture: " << sky_path << " (" << sky_tex->imageWidth << "x" << sky_tex->imageHeight << ")" << std::endl;
    std::cout << "SGA Sun Pixel: (" << sky_tex->sun_pixel_x << ", " << sky_tex->sun_pixel_y << ")" << std::endl;
    std::cout << "True Sun Centroid: (" << true_x << ", " << true_y << ")" << std::endl;
    std::cout << "Sun Direction Angular Error: " << err_deg << " deg" << std::endl;

    return s;
}

// Scene 2: Point light and default gradient background (no sky texture)
inline SceneData create_pointlight_and_gradient_scene() {
    SceneData s;
    setup_slide9_objects(s);

    color lightgray(0.75f, 0.75f, 0.75f);
    color ambient_light(0.25f, 0.25f, 0.25f);

    // Camera setup (environment_texture_sky = nullptr -> camera renders default sky gradient)
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 700;
    s.cam.samples_per_pixel = 1;
    s.cam.vfov = 20;
    s.cam.lookfrom = point3(0.0f, 50.0f, 30.0f);
    s.cam.lookat = point3(0.0f, -30.0f, -120.0f);
    s.cam.environment_texture_sky = nullptr;

    // Point light with ambient component
    point_light* pt_light = new point_light(point3(20.0f, 50.0f, -40.0f), ambient_light, lightgray, lightgray);
    s.lights.add(pt_light);
    s.allocated_lights.push_back(pt_light);

    return s;
}

// Homework 8 Diagnostic Scene: sphere, cylinder, and cube without texture (solid Phong materials)
inline SceneData create_solid_with_textures_scene() {
    SceneData s;

    // Camera setup framed on all three primitives side by side
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 800;
    s.cam.samples_per_pixel = 1;
    s.cam.vfov = 26.0f;
    s.cam.lookfrom = point3(0.0f, 12.0f, 35.0f);
    s.cam.lookat = point3(0.0f, -2.0f, -40.0f);
    s.cam.environment_texture_sky = nullptr;

    // Neutral floor plane at y = -10
    material* floor_mat = new material();
    floor_mat->ka = color(0.12f, 0.12f, 0.12f);
    floor_mat->kd = color(0.45f, 0.45f, 0.48f);
    floor_mat->ks = color(0.08f, 0.08f, 0.08f);
    floor_mat->alpha = 10.0f;
    s.materials.push_back(floor_mat);

    auto floor_plane = make_shared<plane>(point3(0.0f, -10.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), floor_mat);
    s.world.add(floor_plane);

    // Test grid texture from course slides
    texture* grid_tex = new image_texture("../models/grid_A1_D4.png");
    s.textures.push_back(grid_tex);

    // 1. Sphere (shows the grid texture via sphere::hit UVs)
    material* sphere_mat = new material();
    sphere_mat->tex = grid_tex;
    sphere_mat->ka = color(0.2f, 0.2f, 0.2f);
    sphere_mat->kd = color(0.85f, 0.85f, 0.85f);
    sphere_mat->ks = color(0.35f, 0.35f, 0.35f);
    sphere_mat->alpha = 32.0f;
    s.materials.push_back(sphere_mat);

    auto sphere_obj = make_shared<sphere>(point3(-18.0f, -2.0f, -40.0f), 8.0f, sphere_mat);
    s.world.add(sphere_obj);

    // 2. Cylinder (shows the grid texture via cylinder::hit UVs)
    material* cyl_mat = new material();
    cyl_mat->tex = grid_tex;
    cyl_mat->ka = color(0.2f, 0.2f, 0.2f);
    cyl_mat->kd = color(0.85f, 0.85f, 0.85f);
    cyl_mat->ks = color(0.35f, 0.35f, 0.35f);
    cyl_mat->alpha = 32.0f;
    s.materials.push_back(cyl_mat);

    cylinder* cyl_raw = new cylinder(-8.0f, 8.0f, 6.0f, cyl_mat);
    s.allocated_hittables.push_back(cyl_raw);

    auto cyl_inst = make_shared<instance>(cyl_raw, cyl_mat);
    cyl_inst->translate(0.0f, -2.0f, -40.0f);
    s.world.add(cyl_inst);

    // 3. Cube / Box (Solid Gold/Amber, rotated so three faces are clearly visible, no texture)
    material* box_mat = new material();
    box_mat->tex = grid_tex;
    box_mat->ka = color(0.18f, 0.14f, 0.04f);
    box_mat->kd = color(0.90f, 0.65f, 0.15f);
    box_mat->ks = color(0.4f, 0.4f, 0.4f);
    box_mat->alpha = 32.0f;
    s.materials.push_back(box_mat);

    box* box_raw = new box(point3(-6.0f, -6.0f, -6.0f), point3(6.0f, 6.0f, 6.0f), box_mat);
    s.allocated_hittables.push_back(box_raw);

    auto box_inst = make_shared<instance>(box_raw, box_mat);
    box_inst->rotate_y(32.0f);
    box_inst->translate(18.0f, -4.0f, -40.0f);
    s.world.add(box_inst);

    // Lighting: directional key light + point fill
    color lightgray(0.75f, 0.75f, 0.75f);
    color ambient_light(0.15f, 0.15f, 0.15f);

    directional_light* key_light = new directional_light(vec3(0.5f, 1.0f, 0.5f), ambient_light, lightgray, lightgray);
    s.lights.add(key_light);
    s.allocated_lights.push_back(key_light);

    point_light* fill_light = new point_light(point3(-25.0f, 25.0f, 10.0f), color(0.02f, 0.02f, 0.02f), color(0.35f, 0.35f, 0.4f), color(0.1f, 0.1f, 0.1f));
    s.lights.add(fill_light);
    s.allocated_lights.push_back(fill_light);

    return s;
}

inline SceneData create_solid_with_custom_textures_scene() {
    SceneData s;

    // Camera setup framed on all three primitives side by side
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 700;
    s.cam.samples_per_pixel = 1;
    s.cam.vfov = 26.0f;
    s.cam.lookfrom = point3(0.0f, 12.0f, 35.0f);
    s.cam.lookat = point3(0.0f, -2.0f, -40.0f);
    s.cam.environment_texture_sky = nullptr;

    // Neutral floor plane at y = -10
    material* floor_mat = new material();
    floor_mat->ka = color(0.12f, 0.12f, 0.12f);
    floor_mat->kd = color(0.45f, 0.45f, 0.48f);
    floor_mat->ks = color(0.08f, 0.08f, 0.08f);
    floor_mat->alpha = 10.0f;
    s.materials.push_back(floor_mat);

    auto floor_plane = make_shared<plane>(point3(0.0f, -10.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), floor_mat);
    s.world.add(floor_plane);

    // Custom textures with repetition/wrapping factors:
    // 1. Sphere with equirectangular landscape texture
    texture* sphere_tex = new image_texture("../models/canyon_sky_small.png", 1.0f, 1.0f);
    s.textures.push_back(sphere_tex);

    material* sphere_mat = new material();
    sphere_mat->tex = sphere_tex;
    sphere_mat->ka = color(0.2f, 0.2f, 0.2f);
    sphere_mat->kd = color(0.85f, 0.85f, 0.85f);
    sphere_mat->ks = color(0.35f, 0.35f, 0.35f);
    sphere_mat->alpha = 32.0f;
    s.materials.push_back(sphere_mat);

    auto sphere_obj = make_shared<sphere>(point3(-18.0f, -2.0f, -40.0f), 8.0f, sphere_mat);
    s.world.add(sphere_obj);

    // 2. Cylinder with cat texture tiled twice horizontally (repeat_u = 2.0)
    texture* cyl_tex = new image_texture("../models/texturecat.jpg", 2.0f, 1.0f);
    s.textures.push_back(cyl_tex);

    material* cyl_mat = new material();
    cyl_mat->tex = cyl_tex;
    cyl_mat->ka = color(0.2f, 0.2f, 0.2f);
    cyl_mat->kd = color(0.85f, 0.85f, 0.85f);
    cyl_mat->ks = color(0.35f, 0.35f, 0.35f);
    cyl_mat->alpha = 32.0f;
    s.materials.push_back(cyl_mat);

    cylinder* cyl_raw = new cylinder(-8.0f, 8.0f, 6.0f, cyl_mat);
    s.allocated_hittables.push_back(cyl_raw);

    auto cyl_inst = make_shared<instance>(cyl_raw, cyl_mat);
    cyl_inst->translate(0.0f, -2.0f, -40.0f);
    s.world.add(cyl_inst);

    // 3. Cube / Box with checkerboard texture tiled 2x2 on each face (repeat_u = 2.0, repeat_v = 2.0)
    texture* box_tex = new image_texture("../models/texturecheckerboard.png", 2.0f, 2.0f);
    s.textures.push_back(box_tex);

    material* box_mat = new material();
    box_mat->tex = box_tex;
    box_mat->ka = color(0.18f, 0.18f, 0.18f);
    box_mat->kd = color(0.9f, 0.9f, 0.9f);
    box_mat->ks = color(0.3f, 0.3f, 0.3f);
    box_mat->alpha = 32.0f;
    s.materials.push_back(box_mat);

    box* box_raw = new box(point3(-6.0f, -6.0f, -6.0f), point3(6.0f, 6.0f, 6.0f), box_mat);
    s.allocated_hittables.push_back(box_raw);

    auto box_inst = make_shared<instance>(box_raw, box_mat);
    box_inst->rotate_y(32.0f);
    box_inst->translate(18.0f, -4.0f, -40.0f);
    s.world.add(box_inst);

    // Lighting: directional key light + point fill
    color lightgray(0.75f, 0.75f, 0.75f);
    color ambient_light(0.15f, 0.15f, 0.15f);

    directional_light* key_light = new directional_light(vec3(0.5f, 1.0f, 0.5f), ambient_light, lightgray, lightgray);
    s.lights.add(key_light);
    s.allocated_lights.push_back(key_light);

    point_light* fill_light = new point_light(point3(-25.0f, 25.0f, 10.0f), color(0.02f, 0.02f, 0.02f), color(0.35f, 0.35f, 0.4f), color(0.1f, 0.1f, 0.1f));
    s.lights.add(fill_light);
    s.allocated_lights.push_back(fill_light);

    return s;
}
