#include <string>
#include "scenes.h"
#include "spline.h"
#include "point3_list.h"
#include "cylinder.h"

color lightgray(0.75f, 0.75f, 0.75f);
color ambient_light(0.25f, 0.25f, 0.25f);

const vec3 coyote_scale = vec3(0.0355f, 0.0355f, 0.0355f); // 10% reduction from 0.045f


//Helper functions

inline void setup_camera(SceneData& s, point3 lookfrom = point3(0.0f, 0.0f, 0.0f), point3 lookat = point3(0.0f, 0.0f, 1.0f), float fov = 60.0f)
{
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 960;
    s.cam.samples_per_pixel = 48;
    s.cam.vfov = fov;
    s.cam.lookfrom = lookfrom;
    s.cam.lookat = lookat;
    
}

inline void setup_sky(SceneData& s)
{
    environment_texture* sky_tex = new environment_texture("../models/cartoon_canyon_sky_equirectangular_4k.png");
    s.cam.environment_texture_sky = sky_tex;
    s.textures.push_back(sky_tex);
        
    directional_light* dir_light = new directional_light(sky_tex->sun_direction, ambient_light, lightgray, lightgray);
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);
}

inline material* setup_material(SceneData& s, const std::string& texFilename = "", float rep_u = 1.0f, float rep_v = 1.0f)
{
    material* mat = new material();
    mat->ka = color(0.25f, 0.25f, 0.25f);
    mat->kd = lightgray;
    mat->ks = color(0.08f, 0.08f, 0.08f);
    mat->is_toon = true;
    
    if (!texFilename.empty()) {
        std::string texPath = "../models/" + texFilename;
        texture* tex = new image_texture(texPath.c_str(), rep_u, rep_v);
        mat->tex = tex;
        s.textures.push_back(tex);
    }
    
    s.materials.push_back(mat);
    
    return mat;
}

inline shared_ptr<instance> add_instance(SceneData& s, const std::string& objFilename, vec3 scaleVector, vec3 rotVector, vec3 transVector, const std::string& texFilename = "", float rep_u = 1.0f, float rep_v = 1.0f) {
    std::string objPath = "../models/" + objFilename;
    mesh* obj = new mesh(objPath.c_str(), "../models/");
    if (!obj->loaded) {
        std::cerr << "Impossibile caricare " << objPath << std::endl;
    }
    s.meshes.push_back(obj);

    material* mat = setup_material(s, texFilename, rep_u, rep_v);

    auto inst = make_shared<instance>(obj, mat);
    inst->scale(scaleVector.x(), scaleVector.y(), scaleVector.z());
    inst->rotate_x(rotVector.x());
    inst->rotate_y(rotVector.y());
    inst->rotate_z(rotVector.z());
    inst->save_base_transform();
    inst->translate(transVector.x(), transVector.y(), transVector.z());
    s.world.add(inst);
    
    return inst;
}

inline shared_ptr<instance> add_cactus(SceneData& s, vec3 scaleVector, vec3 rotVector, vec3 transVector) {
    auto inst = add_instance(s, "cactus.obj", scaleVector, rotVector, transVector, "cactus.png");
    inst->getMaterial()->is_toon = true;
    inst->getMaterial()->epsilon = 0.32f;
    inst->getMaterial()->K = 3;
    inst->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    return inst;
}

inline shared_ptr<moving_instance> add_moving_instance(
    SceneData& s,
    const std::string& objFilename,
    vec3 scaleVector,
    vec3 baseRotVector,
    vec3 trans0 = vec3(0.0f, 0.0f, 0.0f),
    vec3 trans1 = vec3(0.0f, 0.0f, 0.0f),
    float rot_y0 = 0.0f,
    float rot_y1 = 0.0f,
    const std::string& texFilename = "",
    float rep_u = 1.0f,
    float rep_v = 1.0f)
{
    std::string objPath = "../models/" + objFilename;
    mesh* obj = new mesh(objPath.c_str(), "../models/");
    if (!obj->loaded) {
        std::cerr << "Impossibile caricare " << objPath << std::endl;
    }
    s.meshes.push_back(obj);

    material* mat = setup_material(s, texFilename, rep_u, rep_v);

    auto inst = make_shared<moving_instance>(
        obj, mat,
        0.0f, 0.5f,
        trans0, trans1,
        rot_y0, rot_y1,
        scaleVector, baseRotVector
    );
    s.world.add(inst);

    return inst;
}

// Scenes from script

inline SceneData initial_scene()
{
// camera pointing to the sky, a sphere with moon texture, an airplane passing by. Duration: 3 seconds;
    SceneData s;
    
    setup_camera(s, point3(0.0f, 0.0f, 0.0f), point3(0.0f, 1.0f, 2.0f), 40);
    setup_sky(s);
    s.wait(3.0f);

    material* moon_mat = setup_material(s, "moon.png");
    moon_mat->ks = color(0.0f, 0.0f, 0.0f);
    auto moon = make_shared<sphere>(point3(-22.0f, 48.0f, 60.0f), 4.0f, moon_mat);
    s.world.add(moon);
    
    auto plane = add_instance(s, "f-16.obj", vec3(0.5f, 0.5f, 0.5f), vec3(0.0f, -90.0f, 0.0f), vec3(20.0f, 13.7f, 20.0f));
    
    point3_list plane_control_points;
    plane_control_points.add(point3(20.0f, 13.7f, 20.0f));
    plane_control_points.add(point3(6.67f, 13.7f, 20.0f));
    plane_control_points.add(point3(-6.67f, 13.7f, 20.0f));
    plane_control_points.add(point3(-20.0f, 13.7f, 20.0f));
    
    float plane_dt = 1.0f / s.framerate;
    path plane_path = catmull_rom_interpolate(plane_control_points, plane_dt, false);
    
    for (point3 p : plane_path.points)
    {
        InstanceTransformations transform;
        transform.inst = plane;
        transform.setTranslation(p.x(), p.y(), p.z());
        s.transforms.push_back(transform);
    }
    
    return s;
}

inline SceneData closeup_cactus_scene()
{
// close-up of a cactus, in front of the sun. Camera moving around the cactus. Duration: 3 seconds.
    
    SceneData s;
    
    point3 focus_point(0.0f, 2.15f, 0.35f);

    // Closer camera (macro framing) with reduced FOV (22 degrees)
    setup_camera(s, point3(1.80f, 2.15f, 1.61f), focus_point, 35.0f);
    setup_sky(s);

    // Hero cactus at origin
    auto cactus = add_cactus(s, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f));

    // Perched crow on the front branch of the hero cactus
    auto crow = add_instance(s, "crow.obj", vec3(0.008f, 0.008f, 0.008f), vec3(0.0f, -40.0f, 0.0f), vec3(0.016f, 2.46f, 0.595f), "crow.png");
    crow->getMaterial()->is_toon = true;
    crow->getMaterial()->epsilon = 0.30f;
    crow->getMaterial()->K = 3;
    crow->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);

    // Additional cacti filling horizontal space across midground and background
    add_cactus(s, vec3(1.1f, 1.1f, 1.1f), vec3(0.0f, 25.0f, 0.0f), vec3(1.8f, 0.0f, -0.6f));
    add_cactus(s, vec3(1.3f, 1.3f, 1.3f), vec3(0.0f, -40.0f, 0.0f), vec3(3.2f, 0.0f, -1.5f));
    add_cactus(s, vec3(1.05f, 1.05f, 1.05f), vec3(0.0f, -30.0f, 0.0f), vec3(-1.2f, 0.0f, 1.6f));
    add_cactus(s, vec3(1.25f, 1.25f, 1.25f), vec3(0.0f, 50.0f, 0.0f), vec3(-2.5f, 0.0f, 2.4f));
    add_cactus(s, vec3(1.4f, 1.4f, 1.4f), vec3(0.0f, 15.0f, 0.0f), vec3(0.8f, 0.0f, -3.2f));
    add_cactus(s, vec3(1.35f, 1.35f, 1.35f), vec3(0.0f, -70.0f, 0.0f), vec3(-1.5f, 0.0f, -2.8f));
    add_cactus(s, vec3(0.9f, 0.9f, 0.9f), vec3(0.0f, 110.0f, 0.0f), vec3(0.5f, 0.0f, -1.5f));
    add_cactus(s, vec3(0.85f, 0.85f, 0.85f), vec3(0.0f, -15.0f, 0.0f), vec3(-1.8f, 0.0f, 0.2f));

    // Point light behind camera for fair frontal lighting on cacti and crow
    point_light* cam_light = new point_light(
        point3(4.5f, 3.5f, 4.2f),
        color(0.15f, 0.15f, 0.15f),
        color(0.85f, 0.85f, 0.82f),
        color(0.15f, 0.15f, 0.15f)
    );
    s.lights.add(cam_light);
    s.allocated_lights.push_back(cam_light);

    // Smooth macro orbital arc around cactus and perched crow
    point3_list camera_control_points;
    camera_control_points.add(point3(1.99f, 2.15f, 1.28f));
    camera_control_points.add(point3(1.80f, 2.15f, 1.61f));
    camera_control_points.add(point3(1.56f, 2.15f, 1.91f));
    camera_control_points.add(point3(1.26f, 2.15f, 2.15f));

    float camera_dt = 1.0f / s.framerate;
    path camera_path = catmull_rom_interpolate(camera_control_points, camera_dt, false);

    s.camera_positions.add(camera_path);

    return s;
}

inline SceneData rolling_tumbleweed_scene()
{
    // canyon scene with cacti and rocks, a tumbleweed rolls by. Duration: 5 seconds.
    SceneData s;
   
    // Ground plane with cartoon desert texture
    texture* ground_tex = new image_texture("../models/cartoon_desert_ground.png", 6.0f, 6.0f);
    s.textures.push_back(ground_tex);
    material* ground_mat = new material();
    ground_mat->tex = ground_tex;
    ground_mat->ka = color(0.35f, 0.35f, 0.35f);
    ground_mat->kd = color(0.85f, 0.85f, 0.85f);
    ground_mat->ks = color(0.05f, 0.05f, 0.05f);
    ground_mat->is_toon = false; // Disable toon silhouette on infinite ground plane to avoid horizon blackout
    ground_mat->epsilon = 0.0f;
    s.materials.push_back(ground_mat);

    auto ground_plane = make_shared<plane>(point3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), ground_mat);
    s.world.add(ground_plane);
    
    // Left side foreground/midground cacti & rock cluster
    add_cactus(s, vec3(1.4f, 1.4f, 1.4f), vec3(0.0f, 20.0f, 0.0f), vec3(7.0f, 0.0f, 12.0f));
    add_cactus(s, vec3(0.9f, 0.9f, 0.9f), vec3(0.0f, -35.0f, 0.0f), vec3(8.8f, 0.0f, 13.5f));
    auto r1 = add_instance(s, "rock1.obj", vec3(1.0f, 0.9f, 1.0f), vec3(0.0f, 35.0f, 0.0f), vec3(5.6f, 0.35f, 11.5f));
    r1->getMaterial()->kd = color(0.72f, 0.48f, 0.32f);
    auto r3 = add_instance(s, "rock3.obj", vec3(1.2f, 1.0f, 1.2f), vec3(0.0f, -15.0f, 0.0f), vec3(7.8f, 0.30f, 12.5f));
    r3->getMaterial()->kd = color(0.68f, 0.44f, 0.30f);
    auto r5_fg_left = add_instance(s, "rock5.obj", vec3(1.0f, 0.9f, 1.0f), vec3(0.0f, 45.0f, 0.0f), vec3(9.5f, 0.40f, 14.0f));
    r5_fg_left->getMaterial()->kd = color(0.70f, 0.46f, 0.32f);
    auto r5_fg_near = add_instance(s, "rock5.obj", vec3(0.5f, 0.5f, 0.5f), vec3(0.0f, 10.0f, 0.0f), vec3(4.2f, 0.20f, 9.5f));
    r5_fg_near->getMaterial()->kd = color(0.74f, 0.50f, 0.34f);

    // Right side foreground/midground cacti & rock cluster
    add_cactus(s, vec3(1.25f, 1.25f, 1.25f), vec3(0.0f, -25.0f, 0.0f), vec3(-7.5f, 0.0f, 15.0f));
    add_cactus(s, vec3(0.85f, 0.85f, 0.85f), vec3(0.0f, 50.0f, 0.0f), vec3(-9.2f, 0.0f, 16.5f));
    add_cactus(s, vec3(0.65f, 0.65f, 0.65f), vec3(0.0f, -10.0f, 0.0f), vec3(-6.2f, 0.0f, 16.0f));
    auto r2 = add_instance(s, "rock2.obj", vec3(1.1f, 1.1f, 1.1f), vec3(0.0f, 15.0f, 0.0f), vec3(-6.0f, 0.95f, 14.0f));
    r2->getMaterial()->kd = color(0.70f, 0.46f, 0.32f);
    auto r5 = add_instance(s, "rock5.obj", vec3(1.2f, 1.0f, 1.2f), vec3(0.0f, 70.0f, 0.0f), vec3(-8.5f, 0.45f, 15.5f));
    r5->getMaterial()->kd = color(0.65f, 0.42f, 0.28f);
    auto r1_right = add_instance(s, "rock1.obj", vec3(0.8f, 0.8f, 0.8f), vec3(0.0f, 20.0f, 0.0f), vec3(-10.0f, 0.28f, 17.0f));
    r1_right->getMaterial()->kd = color(0.68f, 0.44f, 0.30f);
    auto r1_fg = add_instance(s, "rock1.obj", vec3(0.6f, 0.6f, 0.6f), vec3(0.0f, -10.0f, 0.0f), vec3(-4.5f, 0.22f, 10.0f));
    r1_fg->getMaterial()->kd = color(0.72f, 0.48f, 0.32f);

    // Small clipped rocks filling ground gaps on the left
    add_instance(s, "rock3.obj", vec3(1.1f, 0.35f, 0.9f), vec3(0.0f, 25.0f, 0.0f), vec3(3.6f, 0.05f, 13.5f), "desertrock1.jpg");
    add_instance(s, "rock1.obj", vec3(0.6f, 0.30f, 0.5f), vec3(0.0f, -40.0f, 0.0f), vec3(4.8f, 0.02f, 15.5f), "desertrock2.jpg");
    add_instance(s, "rock5.obj", vec3(0.8f, 0.35f, 0.7f), vec3(0.0f, 60.0f, 0.0f), vec3(6.2f, 0.06f, 18.0f), "desertrock3.jpg");
    add_instance(s, "rock3.obj", vec3(1.3f, 0.40f, 1.1f), vec3(0.0f, -10.0f, 0.0f), vec3(9.8f, 0.04f, 19.5f), "desertrock2.jpg");
    add_instance(s, "rock1.obj", vec3(0.7f, 0.35f, 0.6f), vec3(0.0f, 15.0f, 0.0f), vec3(11.2f, 0.08f, 15.0f), "desertrock1.jpg");

    // Small clipped rocks filling ground gaps on the right
    add_instance(s, "rock3.obj", vec3(1.0f, 0.35f, 0.8f), vec3(0.0f, -30.0f, 0.0f), vec3(-3.4f, 0.04f, 13.0f), "desertrock3.jpg");
    add_instance(s, "rock1.obj", vec3(0.5f, 0.25f, 0.5f), vec3(0.0f, 45.0f, 0.0f), vec3(-4.6f, 0.02f, 15.0f), "desertrock1.jpg");
    add_instance(s, "rock5.obj", vec3(0.9f, 0.40f, 0.8f), vec3(0.0f, -20.0f, 0.0f), vec3(-5.8f, 0.06f, 18.5f), "desertrock2.jpg");
    add_instance(s, "rock3.obj", vec3(1.2f, 0.35f, 1.0f), vec3(0.0f, 50.0f, 0.0f), vec3(-8.8f, 0.05f, 19.0f), "desertrock1.jpg");
    add_instance(s, "rock1.obj", vec3(0.8f, 0.35f, 0.7f), vec3(0.0f, -15.0f, 0.0f), vec3(-11.5f, 0.08f, 14.5f), "desertrock3.jpg");

    // Midground left & right ridges (varied canyon textures)
    auto r4_bg = add_instance(s, "rock4.obj", vec3(3.0f, 2.2f, 2.5f), vec3(0.0f, 10.0f, 0.0f), vec3(14.0f, 1.10f, 22.0f), "desertrock1.jpg", 1.5f, 1.5f);
    auto r2_ridge_left = add_instance(s, "rock2.obj", vec3(2.0f, 2.5f, 2.0f), vec3(0.0f, -30.0f, 0.0f), vec3(16.5f, 2.20f, 25.0f), "desertrock2.jpg", 1.5f, 1.5f);
    auto r3_flank_left = add_instance(s, "rock3.obj", vec3(3.5f, 2.5f, 3.0f), vec3(0.0f, 15.0f, 0.0f), vec3(20.0f, 1.30f, 28.0f), "desertrock3.jpg", 1.5f, 1.5f);
    add_cactus(s, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 45.0f, 0.0f), vec3(15.0f, 0.0f, 23.0f));

    auto r4_ridge_right = add_instance(s, "rock4.obj", vec3(3.2f, 2.0f, 2.8f), vec3(0.0f, -25.0f, 0.0f), vec3(-13.5f, 1.00f, 23.0f), "desertrock3.jpg", 1.5f, 1.5f);
    add_cactus(s, vec3(0.85f, 0.85f, 0.85f), vec3(0.0f, -50.0f, 0.0f), vec3(-12.0f, 0.0f, 21.0f));

    // Central midground cacti (with toon shading and distinct silhouettes)
    add_cactus(s, vec3(1.15f, 1.15f, 1.15f), vec3(0.0f, 30.0f, 0.0f), vec3(-2.2f, 0.0f, 26.0f));
    add_cactus(s, vec3(0.95f, 0.95f, 0.95f), vec3(0.0f, -25.0f, 0.0f), vec3(2.5f, 0.0f, 29.0f));
    add_cactus(s, vec3(0.80f, 0.80f, 0.80f), vec3(0.0f, 15.0f, 0.0f), vec3(0.4f, 0.0f, 34.0f));
    add_cactus(s, vec3(0.70f, 0.70f, 0.70f), vec3(0.0f, -40.0f, 0.0f), vec3(-3.5f, 0.0f, 38.0f));
    add_cactus(s, vec3(0.85f, 0.85f, 0.85f), vec3(0.0f, 10.0f, 0.0f), vec3(-0.8f, 0.0f, 44.0f));
    add_cactus(s, vec3(0.65f, 0.65f, 0.65f), vec3(0.0f, 50.0f, 0.0f), vec3(3.8f, 0.0f, 42.0f));
    add_instance(s, "rock1.obj", vec3(0.5f, 0.25f, 0.5f), vec3(0.0f, 20.0f, 0.0f), vec3(-1.8f, 0.04f, 26.5f), "desertrock2.jpg");
    add_instance(s, "rock5.obj", vec3(0.6f, 0.30f, 0.5f), vec3(0.0f, -30.0f, 0.0f), vec3(2.8f, 0.05f, 29.5f), "desertrock1.jpg");

    // Very distant background canyon formations across the horizon (Z: 95-125)
    // Varied canyon textures (desertrock1.jpg, desertrock2.jpg, desertrock3.jpg)

    // Far-left distant ridge & spires (extended offscreen to eliminate any bottom-left sky corner gap)
    auto r4_dist_left_edge = add_instance(s, "rock4.obj", vec3(45.0f, 26.0f, 32.0f), vec3(0.0f, -15.0f, 0.0f), vec3(85.0f, 13.0f, 105.0f), "desertrock2.jpg", 2.0f, 2.0f);
    auto r5_ridge_left = add_instance(s, "rock5.obj", vec3(36.0f, 22.0f, 28.0f), vec3(0.0f, -15.0f, 0.0f), vec3(70.0f, 10.0f, 122.0f), "desertrock1.jpg", 2.0f, 2.0f);
    auto r4_dist_left = add_instance(s, "rock4.obj", vec3(36.0f, 22.0f, 28.0f), vec3(0.0f, -25.0f, 0.0f), vec3(50.0f, 11.0f, 110.0f), "desertrock2.jpg", 2.0f, 2.0f);
    auto r2_spire_left = add_instance(s, "rock2.obj", vec3(18.0f, 28.0f, 18.0f), vec3(0.0f, 35.0f, 0.0f), vec3(32.0f, 25.0f, 95.0f), "desertrock3.jpg", 1.5f, 2.0f);

    // Central distant canyon formations
    auto r4_dist_center = add_instance(s, "rock4.obj", vec3(32.0f, 16.0f, 25.0f), vec3(0.0f, 10.0f, 0.0f), vec3(0.0f, 8.0f, 118.0f), "desertrock2.jpg", 2.0f, 2.0f);
    auto r3_dist_center_left = add_instance(s, "rock3.obj", vec3(22.0f, 12.0f, 18.0f), vec3(0.0f, 40.0f, 0.0f), vec3(-8.0f, 3.0f, 105.0f), "desertrock3.jpg", 2.0f, 2.0f);
    auto r5_dist_center_right = add_instance(s, "rock5.obj", vec3(24.0f, 14.0f, 20.0f), vec3(0.0f, -25.0f, 0.0f), vec3(9.0f, 6.5f, 110.0f), "desertrock1.jpg", 2.0f, 2.0f);

    // Far-right distant ridge & spires
    auto r4_dist_right = add_instance(s, "rock4.obj", vec3(38.0f, 24.0f, 30.0f), vec3(0.0f, 30.0f, 0.0f), vec3(-50.0f, 12.0f, 112.0f), "desertrock3.jpg", 2.0f, 2.0f);
    auto r2_spire_right = add_instance(s, "rock2.obj", vec3(20.0f, 30.0f, 20.0f), vec3(0.0f, -30.0f, 0.0f), vec3(-33.0f, 27.0f, 96.0f), "desertrock2.jpg", 1.5f, 2.0f);
    auto r5_ridge_right = add_instance(s, "rock5.obj", vec3(34.0f, 22.0f, 28.0f), vec3(0.0f, 25.0f, 0.0f), vec3(-68.0f, 10.0f, 125.0f), "desertrock1.jpg", 2.0f, 2.0f);

    // Tumbleweed with motion blur
    auto tumbleweed = add_moving_instance(
        s, "PUSHILIN_Tumbleweed.obj",
        vec3(1.0f, 1.0f, 1.0f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(-0.28f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "PUSHILIN_Tumbleweed.png"
    );

    // Horizontally stretched dust clouds clipping into the ground with cartoon cloud-puff texture
    // 1. Big background dust bank (far, wide horizontal stretch, clipping through dunes)
    auto dust_bg = add_moving_instance(
        s, "dust_cloud.obj",
        vec3(16.0f, 2.5f, 5.0f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(-0.12f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "dust.png",
        3.0f, 1.0f
    );
    dust_bg->getMaterial()->is_toon = true;
    dust_bg->getMaterial()->epsilon = 0.28f;
    dust_bg->getMaterial()->K = 3;
    dust_bg->getMaterial()->ka = color(0.38f, 0.23f, 0.11f);
    dust_bg->getMaterial()->kd = color(0.86f, 0.53f, 0.26f);
    dust_bg->getMaterial()->ks = color(0.0f, 0.0f, 0.0f);

    // 2. Midground stretched dust cloud (clipping through ground plane)
    auto dust_mg = add_moving_instance(
        s, "dust_cloud.obj",
        vec3(9.0f, 1.4f, 2.8f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(-0.20f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "dust.png",
        2.0f, 1.0f
    );
    dust_mg->getMaterial()->is_toon = true;
    dust_mg->getMaterial()->epsilon = 0.28f;
    dust_mg->getMaterial()->K = 3;
    dust_mg->getMaterial()->ka = color(0.38f, 0.22f, 0.10f);
    dust_mg->getMaterial()->kd = color(0.87f, 0.52f, 0.24f);
    dust_mg->getMaterial()->ks = color(0.0f, 0.0f, 0.0f);

    // 3. Near ground dust cloud rolling along desert floor behind tumbleweed
    auto dust_near = add_moving_instance(
        s, "dust_cloud.obj",
        vec3(4.5f, 0.85f, 1.8f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(-0.28f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "dust.png",
        1.5f, 1.0f
    );
    dust_near->getMaterial()->is_toon = true;
    dust_near->getMaterial()->epsilon = 0.28f;
    dust_near->getMaterial()->K = 3;
    dust_near->getMaterial()->ka = color(0.36f, 0.21f, 0.10f);
    dust_near->getMaterial()->kd = color(0.87f, 0.50f, 0.22f);
    dust_near->getMaterial()->ks = color(0.0f, 0.0f, 0.0f);

    // Catmull-Rom open spline: 6 control points = 5 segments.
    // dt = 1/30 produces 5 * 30 = 150 points (exactly 5s at 30fps).
    point3_list tumbleweed_cps;
    tumbleweed_cps.add(point3(16.0f, 1.0f, 14.5f));   // outer left (offscreen)
    tumbleweed_cps.add(point3(9.6f, 1.35f, 14.2f));   // entering view, small hop
    tumbleweed_cps.add(point3(3.2f, 1.0f, 14.0f));    // mid-left, touches ground
    tumbleweed_cps.add(point3(-3.2f, 1.45f, 13.8f));  // mid-right bounce
    tumbleweed_cps.add(point3(-9.6f, 1.05f, 13.6f));  // exiting right
    tumbleweed_cps.add(point3(-16.0f, 1.0f, 13.5f));  // outer right (offscreen)

    float dt = 1.0f / s.framerate;
    path tumbleweed_path = catmull_rom_interpolate(tumbleweed_cps, dt, false);

    // Animate tumbleweed and dust clouds with parallax speeds and shutter motion blur
    int total_pts = tumbleweed_path.points.size();
    for (int i = 0; i < total_pts; i++)
    {
        float f = (total_pts > 1) ? (float)i / (float)(total_pts - 1) : 0.0f;

        // 1. Tumbleweed rolling & bouncing with motion blur
        point3 p = tumbleweed_path.points[i];
        float rot_z = f * 1833.5f;
        float rot_y = 25.0f * sin(f * 4.0f * pi);
        float rot_x = 15.0f * sin(f * 3.0f * pi);

        InstanceTransformations trans_tw;
        trans_tw.inst = tumbleweed;
        trans_tw.setTranslation(p.x(), p.y(), p.z());
        trans_tw.rotationVector = vec3(rot_x, rot_y, rot_z);
        trans_tw.has_blur_override = true;
        trans_tw.blur_trans = vec3(-0.28f, 0.0f, 0.0f);
        trans_tw.blur_rot = vec3(0.0f, 0.0f, 12.0f);

        // 2. Background dust bank (Z = 55.0f, clips ground, gentle drift)
        float bg_x = (1.0f - f) * 14.0f + f * (-10.0f);
        float bg_y = 0.80f + 0.10f * sin(f * 2.0f * pi);
        InstanceTransformations trans_bg;
        trans_bg.inst = dust_bg;
        trans_bg.setTranslation(bg_x, bg_y, 55.0f);
        trans_bg.rotationVector = vec3(0.0f, 5.0f * sin(f * pi), 0.0f);
        trans_bg.has_blur_override = true;
        trans_bg.blur_trans = vec3(-0.12f, 0.0f, 0.0f);

        // 3. Midground stretched dust cloud (Z = 28.0f, clips ground, mid parallax)
        float mg_x = (1.0f - f) * 18.0f + f * (-14.0f);
        float mg_y = 0.35f + 0.10f * sin(f * 3.0f * pi);
        InstanceTransformations trans_mg;
        trans_mg.inst = dust_mg;
        trans_mg.setTranslation(mg_x, mg_y, 28.0f);
        trans_mg.rotationVector = vec3(0.0f, 5.0f + 6.0f * f, 0.0f);
        trans_mg.has_blur_override = true;
        trans_mg.blur_trans = vec3(-0.20f, 0.0f, 0.0f);

        // 4. Near dust cloud rolling on ground behind tumbleweed (Z = 16.0f, clips ground)
        float near_x = (1.0f - f) * 20.0f + f * (-22.0f);
        float near_y = 0.15f + 0.06f * sin(f * 4.0f * pi);
        InstanceTransformations trans_near;
        trans_near.inst = dust_near;
        trans_near.setTranslation(near_x, near_y, 16.0f);
        trans_near.rotationVector = vec3(0.0f, 10.0f * f, 0.0f);
        trans_near.has_blur_override = true;
        trans_near.blur_trans = vec3(-0.28f, 0.0f, 0.0f);

        std::vector<InstanceTransformations> frame_vec;
        frame_vec.push_back(trans_tw);
        frame_vec.push_back(trans_bg);
        frame_vec.push_back(trans_mg);
        frame_vec.push_back(trans_near);
        s.frame_transforms.push_back(frame_vec);
    }

    // Pitch camera slightly down (lookat y = 4.0 at z = 15.0) to raise the horizon line to ~57% of viewport
    setup_camera(s, point3(0.0f, 5.0f, 0.0f), point3(0.0f, 4.0f, 15.0f), 50.0f);
    s.cam.t0 = 0.0f;
    s.cam.t1 = 0.5f;
    s.cam.csi = 48.0f;
    s.cam.samples_per_pixel = 48;

    // Center on canyon mountains (rotation_y = 0), zoomed out (zoom = 2.2f) and elevated (pitch_offset = 0.26f)
    environment_texture* sky_tex = new environment_texture("../models/cartoon_canyon_sky_equirectangular_4k.png", 2.2f, 0.0f, 0.26f);
    s.cam.environment_texture_sky = sky_tex;
    s.textures.push_back(sky_tex);

    // Sun shines from over camera's left shoulder (L_z < 0) onto the front of objects (camera points at sunny side)
    vec3 sun_dir = unit_vector(vec3(0.25f, 0.80f, -0.50f));
    directional_light* dir_light = new directional_light(sun_dir, color(0.35f, 0.35f, 0.35f), color(0.95f, 0.92f, 0.85f), color(0.15f, 0.15f, 0.15f));
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);

    s.wait(5.0f);

    return s;
}

inline SceneData turtle_scene_roadrunner()
{
    // Still life matching the storyboard:
    // - group of three cacti
    // - rocks scattered on the ground (to be stacked into a cairn by roadrunner whirlwind)
    // - turtle is slowly walking, trying to push a pixar textured ball
    // - roadrunner passes by at supersonic speed with high translational motion blur
    // - roadrunner whirlwind sweeps up scattered rocks and stacks them into the cairn tower,
    //   and sets the turtle spinning around with rotational motion blur
    // - scene ends the instant roadrunner is out of field (at x = -24.0) to match seamlessly with turtle_scene_coyote
    // Duration: 3.5 seconds (105 frames at 30 fps)
    SceneData s;

    // Camera setup with shutter motion blur ([t0, t1] = [0.0, 0.5], fine csi: 24 shutter instants)
    setup_camera(s, point3(0.0f, 3.8f, 2.0f), point3(0.0f, 2.0f, 13.5f), 48.0f);
    s.cam.t0 = 0.0f;
    s.cam.t1 = 0.5f;
    s.cam.csi = 24.0f;
    s.cam.samples_per_pixel = 24;

    // Ground plane with cartoon desert texture
    texture* ground_tex = new image_texture("../models/cartoon_desert_ground.png", 6.0f, 6.0f);
    s.textures.push_back(ground_tex);
    material* ground_mat = new material();
    ground_mat->tex = ground_tex;
    ground_mat->ka = color(0.35f, 0.35f, 0.35f);
    ground_mat->kd = color(0.85f, 0.85f, 0.85f);
    ground_mat->ks = color(0.05f, 0.05f, 0.05f);
    ground_mat->is_toon = false; // Disable toon silhouette on infinite ground plane to avoid horizon blackout
    ground_mat->epsilon = 0.0f;
    s.materials.push_back(ground_mat);

    auto ground_plane = make_shared<plane>(point3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), ground_mat);
    s.world.add(ground_plane);

    // Distant background canyon formations (Z: 95-125)
    add_instance(s, "rock4.obj", vec3(36.0f, 22.0f, 28.0f), vec3(0.0f, -25.0f, 0.0f), vec3(48.0f, 11.0f, 110.0f), "desertrock2.jpg", 2.0f, 2.0f);
    add_instance(s, "rock2.obj", vec3(18.0f, 28.0f, 18.0f), vec3(0.0f, 35.0f, 0.0f), vec3(32.0f, 25.0f, 95.0f), "desertrock3.jpg", 1.5f, 2.0f);
    add_instance(s, "rock4.obj", vec3(32.0f, 16.0f, 25.0f), vec3(0.0f, 10.0f, 0.0f), vec3(0.0f, 8.0f, 118.0f), "desertrock2.jpg", 2.0f, 2.0f);
    add_instance(s, "rock4.obj", vec3(38.0f, 24.0f, 30.0f), vec3(0.0f, 30.0f, 0.0f), vec3(-50.0f, 12.0f, 112.0f), "desertrock3.jpg", 2.0f, 2.0f);
    add_instance(s, "rock2.obj", vec3(20.0f, 30.0f, 20.0f), vec3(0.0f, -30.0f, 0.0f), vec3(-33.0f, 27.0f, 96.0f), "desertrock2.jpg", 1.5f, 2.0f);

    // 1. Group of three cacti
    add_instance(s, "cactus.obj", vec3(1.35f, 1.35f, 1.35f), vec3(0.0f, 20.0f, 0.0f), vec3(-4.4f, 0.0f, 14.8f), "cactus.png");
    add_instance(s, "cactus.obj", vec3(0.95f, 0.95f, 0.95f), vec3(0.0f, -35.0f, 0.0f), vec3(-3.2f, 0.0f, 15.5f), "cactus.png");
    add_instance(s, "cactus.obj", vec3(0.70f, 0.70f, 0.70f), vec3(0.0f, 45.0f, 0.0f), vec3(-5.2f, 0.0f, 15.0f), "cactus.png");

    // 2. Base rock and accent rocks around cairn site
    auto rock_base = add_instance(s, "rock3.obj", vec3(1.35f, 0.65f, 1.25f), vec3(0.0f, 10.0f, 0.0f), vec3(3.2f, 0.25f, 13.8f), "desertrock1.jpg");
    add_instance(s, "rock5.obj", vec3(0.55f, 0.32f, 0.50f), vec3(0.0f, 40.0f, 0.0f), vec3(2.0f, 0.15f, 13.2f), "desertrock2.jpg");
    add_instance(s, "rock1.obj", vec3(0.45f, 0.25f, 0.42f), vec3(0.0f, -20.0f, 0.0f), vec3(4.2f, 0.12f, 14.2f), "desertrock3.jpg");

    // 3. Rocks to be stacked by the whirlwind (moving_instance)
    // Initial scattered positions on desert ground
    vec3 mid_start_pos(4.9f, 0.28f, 14.1f);
    vec3 mid_start_rot(8.0f, 50.0f, -12.0f);
    vec3 mid_target_pos(3.15f, 0.92f, 13.8f);
    vec3 mid_target_rot(0.0f, -25.0f, 0.0f);
    auto rock_mid = add_moving_instance(
        s, "rock5.obj",
        vec3(1.05f, 0.85f, 0.95f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "desertrock2.jpg"
    );
    rock_mid->frame_trans = mid_start_pos;

    vec3 top_start_pos(1.7f, 0.22f, 13.5f);
    vec3 top_start_rot(-15.0f, -60.0f, 25.0f);
    vec3 top_target_pos(3.10f, 1.62f, 13.8f);
    vec3 top_target_rot(0.0f, 35.0f, 0.0f);
    auto rock_top = add_moving_instance(
        s, "rock1.obj",
        vec3(0.80f, 0.68f, 0.80f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "desertrock3.jpg"
    );
    rock_top->frame_trans = top_start_pos;

    vec3 tip_start_pos(2.5f, 0.18f, 14.7f);
    vec3 tip_start_rot(20.0f, 85.0f, -5.0f);
    vec3 tip_target_pos(3.08f, 2.22f, 13.8f);
    vec3 tip_target_rot(0.0f, -15.0f, 0.0f);
    auto rock_tip = add_moving_instance(
        s, "rock2.obj",
        vec3(0.42f, 0.52f, 0.42f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "desertrock1.jpg"
    );
    rock_tip->frame_trans = tip_start_pos;

    // 4. Pixar textured ball
    material* ball_mat = setup_material(s, "pixar_ball.png");
    ball_mat->ks = color(0.20f, 0.20f, 0.20f);
    sphere* raw_ball = new sphere(point3(0.0f, 0.0f, 0.0f), 0.60f, ball_mat);
    s.allocated_hittables.push_back(raw_ball);
    auto ball = make_shared<moving_instance>(
        raw_ball, ball_mat,
        0.0f, 0.5f,
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f)
    );
    ball->frame_trans = vec3(1.0f, 0.60f, 12.2f);
    s.world.add(ball);

    // 5. Turtle slowly walking, trying to push pixar ball, then spinning in whirlwind
    auto turtle = add_moving_instance(
        s, "turtle.obj",
        vec3(1.35f, 1.35f, 1.35f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "turtle.png"
    );
    turtle->frame_trans = vec3(-1.2f, 0.35f, 13.0f);

    // 6. Roadrunner passing by at supersonic speed with high translational motion blur
    vec3 rr_start_pos(24.0f, 1.81f, 8.0f);
    auto roadrunner = add_moving_instance(
        s, "roadrunner_running.obj",
        vec3(0.045f, 0.045f, 0.045f), // scale
        vec3(0.0f, -90.0f, 0.0f),     // pointed along -X
        vec3(0.0f, 0.0f, 0.0f), vec3(-2.8f, 0.0f, 0.0f), // supersonic blur along -X
        0.0f, 0.0f,
        "roadrunner.png"
    );
    roadrunner->frame_trans = rr_start_pos;

    // Sky texture and lighting
    environment_texture* sky_tex = new environment_texture("../models/cartoon_canyon_sky_equirectangular_4k.png", 2.2f, 0.0f, 0.26f);
    s.cam.environment_texture_sky = sky_tex;
    s.textures.push_back(sky_tex);

    vec3 sun_dir = unit_vector(vec3(0.25f, 0.80f, -0.50f));
    directional_light* dir_light = new directional_light(sun_dir, color(0.35f, 0.35f, 0.35f), color(0.95f, 0.92f, 0.85f), color(0.15f, 0.15f, 0.15f));
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);

    // Fill light from camera direction
    directional_light* fill_light = new directional_light(unit_vector(vec3(0.0f, 2.0f, -1.0f)), color(0.0f, 0.0f, 0.0f), color(0.40f, 0.40f, 0.40f), color(0.05f, 0.05f, 0.05f));
    s.lights.add(fill_light);
    s.allocated_lights.push_back(fill_light);

    // Duration: 3.5 seconds (105 frames at 30 fps)
    s.wait(3.5f);

    int total_frames = ceil(s.framerate * 3.5f); // 105 frames (0 to 104)
    for (int i = 0; i < total_frames; i++)
    {
        // 1. Roadrunner: off-screen right (x = 24) until frame 80, then rockets left to x = -24 by frame 104
        float rr_x = 24.0f;
        vec3 rr_blur(0.0f, 0.0f, 0.0f);
        if (i >= 81) {
            float rr_u = (float)(i - 81) / (float)(total_frames - 1 - 81); // 0.0 to 1.0
            rr_x = (1.0f - rr_u) * 24.0f + rr_u * (-24.0f);
            rr_blur = vec3(-2.8f, 0.0f, 0.0f);
        }
        InstanceTransformations trans_rr;
        trans_rr.inst = roadrunner;
        trans_rr.setTranslation(rr_x, 1.81f, 8.0f);
        trans_rr.rotationVector = vec3(0.0f, 0.0f, 0.0f);
        trans_rr.has_blur_override = true;
        trans_rr.blur_trans = rr_blur;

        // 2. Ball: pushed slowly forward from x = 1.0 to 1.7 by frame 80, then rests at 1.7
        float ball_x = 1.7f;
        float ball_rot_z = -((1.7f - 1.0f) / 0.60f) * (180.0f / pi);
        vec3 ball_blur(0.0f, 0.0f, 0.0f);
        if (i < 80) {
            float u = (float)i / 80.0f;
            ball_x = (1.0f - u) * 1.0f + u * 1.7f;
            ball_rot_z = -((ball_x - 1.0f) / 0.60f) * (180.0f / pi);
            ball_blur = vec3(0.005f, 0.0f, 0.0f);
        }
        InstanceTransformations trans_ball;
        trans_ball.inst = ball;
        trans_ball.setTranslation(ball_x, 0.60f, 12.2f);
        trans_ball.rotationVector = vec3(0.0f, 0.0f, ball_rot_z);
        trans_ball.has_blur_override = true;
        trans_ball.blur_trans = ball_blur;

        // 3. Turtle: walks from x = -1.2 to -0.5 (frame 0-80), pauses (80-91),
        // then when roadrunner passes (i >= 92), whirlwind whips turtle into rapid spin around Y
        float turtle_x = -0.5f;
        float turtle_pitch = 0.0f;
        float turtle_roll = 0.0f;
        float turtle_yaw = 0.0f;
        float turtle_blur_yaw = 0.0001f;
        vec3 turtle_blur_trans(0.0f, 0.0f, 0.0f);

        if (i < 80) {
            float u = (float)i / 80.0f;
            turtle_x = (1.0f - u) * (-1.2f) + u * (-0.5f);
            turtle_pitch = 1.5f * cos(u * 16.0f * pi);
            turtle_roll = 1.5f * sin(u * 16.0f * pi);
            turtle_blur_trans = vec3(0.005f, 0.0f, 0.0f);
        } else if (i < 92) {
            turtle_x = -0.5f;
            turtle_pitch = 0.0f;
            turtle_roll = 0.0f;
            turtle_yaw = 0.0f;
            turtle_blur_yaw = 0.0001f;
        } else {
            turtle_x = -0.5f;
            float spin_u = (float)(i - 92) / (float)(total_frames - 1 - 92); // 0.0 to 1.0
            turtle_yaw = 720.0f * spin_u * spin_u; // accelerates to 720 deg
            turtle_blur_yaw = 60.0f * spin_u * spin_u + 0.0001f; // reaches 60 deg blur at cut
        }

        InstanceTransformations trans_turtle;
        trans_turtle.inst = turtle;
        trans_turtle.setTranslation(turtle_x, 0.35f, 13.0f);
        trans_turtle.rotationVector = vec3(turtle_pitch, turtle_yaw, turtle_roll);
        trans_turtle.has_blur_override = true;
        trans_turtle.blur_trans = turtle_blur_trans;
        trans_turtle.blur_rot = vec3(0.0f, turtle_blur_yaw, 0.0f);
        trans_turtle.blur_rot_y = turtle_blur_yaw;

        // 4. Rocks stacking up (rock_mid, rock_top, rock_tip):
        // Lifted by roadrunner whirlwind, arcing onto rock_base to form cairn
        // Mid rock flight: frames 88-94
        vec3 mid_pos = mid_start_pos;
        vec3 mid_rot = mid_start_rot;
        vec3 mid_btrans(0.0f, 0.0f, 0.0f);
        vec3 mid_brot(0.0f, 0.0f, 0.0f);
        if (i >= 88 && i <= 94) {
            float u = (float)(i - 88) / 6.0f;
            float su = u * u * (3.0f - 2.0f * u);
            mid_pos = (1.0f - su) * mid_start_pos + su * mid_target_pos;
            mid_pos[1] += 1.4f * 4.0f * u * (1.0f - u);
            mid_rot = (1.0f - su) * mid_start_rot + su * mid_target_rot + vec3(u * 180.0f, u * 120.0f, 0.0f);
            mid_btrans = vec3(-0.3f, 0.4f * (1.0f - 2.0f * u), 0.0f);
            mid_brot = vec3(15.0f, 25.0f, 10.0f);
        } else if (i > 94) {
            mid_pos = mid_target_pos;
            mid_rot = mid_target_rot;
        }
        InstanceTransformations trans_mid;
        trans_mid.inst = rock_mid;
        trans_mid.setTranslation(mid_pos.x(), mid_pos.y(), mid_pos.z());
        trans_mid.rotationVector = mid_rot;
        trans_mid.has_blur_override = true;
        trans_mid.blur_trans = mid_btrans;
        trans_mid.blur_rot = mid_brot;

        // Top rock flight: frames 90-97
        vec3 top_pos = top_start_pos;
        vec3 top_rot = top_start_rot;
        vec3 top_btrans(0.0f, 0.0f, 0.0f);
        vec3 top_brot(0.0f, 0.0f, 0.0f);
        if (i >= 90 && i <= 97) {
            float u = (float)(i - 90) / 7.0f;
            float su = u * u * (3.0f - 2.0f * u);
            top_pos = (1.0f - su) * top_start_pos + su * top_target_pos;
            top_pos[1] += 1.8f * 4.0f * u * (1.0f - u);
            top_rot = (1.0f - su) * top_start_rot + su * top_target_rot + vec3(u * 220.0f, -u * 150.0f, 0.0f);
            top_btrans = vec3(0.2f, 0.5f * (1.0f - 2.0f * u), 0.0f);
            top_brot = vec3(20.0f, -30.0f, 15.0f);
        } else if (i > 97) {
            top_pos = top_target_pos;
            top_rot = top_target_rot;
        }
        InstanceTransformations trans_top;
        trans_top.inst = rock_top;
        trans_top.setTranslation(top_pos.x(), top_pos.y(), top_pos.z());
        trans_top.rotationVector = top_rot;
        trans_top.has_blur_override = true;
        trans_top.blur_trans = top_btrans;
        trans_top.blur_rot = top_brot;

        // Tip rock flight: frames 92-100
        vec3 tip_pos = tip_start_pos;
        vec3 tip_rot = tip_start_rot;
        vec3 tip_btrans(0.0f, 0.0f, 0.0f);
        vec3 tip_brot(0.0f, 0.0f, 0.0f);
        if (i >= 92 && i <= 100) {
            float u = (float)(i - 92) / 8.0f;
            float su = u * u * (3.0f - 2.0f * u);
            tip_pos = (1.0f - su) * tip_start_pos + su * tip_target_pos;
            tip_pos[1] += 2.2f * 4.0f * u * (1.0f - u);
            tip_rot = (1.0f - su) * tip_start_rot + su * tip_target_rot + vec3(-u * 260.0f, u * 190.0f, 0.0f);
            tip_btrans = vec3(0.1f, 0.6f * (1.0f - 2.0f * u), 0.0f);
            tip_brot = vec3(-25.0f, 35.0f, 20.0f);
        } else if (i > 100) {
            tip_pos = tip_target_pos;
            tip_rot = tip_target_rot;
        }
        InstanceTransformations trans_tip;
        trans_tip.inst = rock_tip;
        trans_tip.setTranslation(tip_pos.x(), tip_pos.y(), tip_pos.z());
        trans_tip.rotationVector = tip_rot;
        trans_tip.has_blur_override = true;
        trans_tip.blur_trans = tip_btrans;
        trans_tip.blur_rot = tip_brot;

        std::vector<InstanceTransformations> frame_vec;
        frame_vec.push_back(trans_rr);
        frame_vec.push_back(trans_ball);
        frame_vec.push_back(trans_turtle);
        frame_vec.push_back(trans_mid);
        frame_vec.push_back(trans_top);
        frame_vec.push_back(trans_tip);
        s.frame_transforms.push_back(frame_vec);
    }

    return s;
}


inline SceneData turtle_scene_coyote()
{
    // Same still life from before:
    // - group of three cacti
    // - rocks stacked up one upon another (cairn/rock tower)
    // - pixar textured ball sitting nearby, kicked out to the right with tumbleweed motion and shutter motion blur
    // - turtle spinning with rotational motion blur, capsizing when coyote passes, decelerating while reducing blur to practically zero
    // - coyote strapped to ACME rocket closer to camera (z = 8.0) passing by with high translational motion blur
    // Duration: 3 seconds (90 frames at 30 fps)
    SceneData s;

    // Camera setup with shutter motion blur ([t0, t1] = [0.0, 0.5], fine csi: 24 shutter instants)
    setup_camera(s, point3(0.0f, 3.8f, 2.0f), point3(0.0f, 2.0f, 13.5f), 48.0f);
    s.cam.t0 = 0.0f;
    s.cam.t1 = 0.5f;
    s.cam.csi = 24.0f;
    s.cam.samples_per_pixel = 24;

    // Ground plane with cartoon desert texture
    texture* ground_tex = new image_texture("../models/cartoon_desert_ground.png", 6.0f, 6.0f);
    s.textures.push_back(ground_tex);
    material* ground_mat = new material();
    ground_mat->tex = ground_tex;
    ground_mat->ka = color(0.35f, 0.35f, 0.35f);
    ground_mat->kd = color(0.85f, 0.85f, 0.85f);
    ground_mat->ks = color(0.05f, 0.05f, 0.05f);
    ground_mat->is_toon = false; // Disable toon silhouette on infinite ground plane to avoid horizon blackout
    ground_mat->epsilon = 0.0f;
    s.materials.push_back(ground_mat);

    auto ground_plane = make_shared<plane>(point3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), ground_mat);
    s.world.add(ground_plane);

    // Distant background canyon formations (Z: 95-125)
    add_instance(s, "rock4.obj", vec3(36.0f, 22.0f, 28.0f), vec3(0.0f, -25.0f, 0.0f), vec3(48.0f, 11.0f, 110.0f), "desertrock2.jpg", 2.0f, 2.0f);
    add_instance(s, "rock2.obj", vec3(18.0f, 28.0f, 18.0f), vec3(0.0f, 35.0f, 0.0f), vec3(32.0f, 25.0f, 95.0f), "desertrock3.jpg", 1.5f, 2.0f);
    add_instance(s, "rock4.obj", vec3(32.0f, 16.0f, 25.0f), vec3(0.0f, 10.0f, 0.0f), vec3(0.0f, 8.0f, 118.0f), "desertrock2.jpg", 2.0f, 2.0f);
    add_instance(s, "rock4.obj", vec3(38.0f, 24.0f, 30.0f), vec3(0.0f, 30.0f, 0.0f), vec3(-50.0f, 12.0f, 112.0f), "desertrock3.jpg", 2.0f, 2.0f);
    add_instance(s, "rock2.obj", vec3(20.0f, 30.0f, 20.0f), vec3(0.0f, -30.0f, 0.0f), vec3(-33.0f, 27.0f, 96.0f), "desertrock2.jpg", 1.5f, 2.0f);


    // 1. Group of three cacti
    add_instance(s, "cactus.obj", vec3(1.35f, 1.35f, 1.35f), vec3(0.0f, 20.0f, 0.0f), vec3(-4.4f, 0.0f, 14.8f), "cactus.png");
    add_instance(s, "cactus.obj", vec3(0.95f, 0.95f, 0.95f), vec3(0.0f, -35.0f, 0.0f), vec3(-3.2f, 0.0f, 15.5f), "cactus.png");
    add_instance(s, "cactus.obj", vec3(0.70f, 0.70f, 0.70f), vec3(0.0f, 45.0f, 0.0f), vec3(-5.2f, 0.0f, 15.0f), "cactus.png");

    // 2. Rocks stacked up one upon another (cairn/rock tower on right)
    auto rock_base = add_instance(s, "rock3.obj", vec3(1.35f, 0.65f, 1.25f), vec3(0.0f, 10.0f, 0.0f), vec3(3.2f, 0.25f, 13.8f), "desertrock1.jpg");
    auto rock_mid = add_instance(s, "rock5.obj", vec3(1.05f, 0.85f, 0.95f), vec3(0.0f, -25.0f, 0.0f), vec3(3.15f, 0.92f, 13.8f), "desertrock2.jpg");
    auto rock_top = add_instance(s, "rock1.obj", vec3(0.80f, 0.68f, 0.80f), vec3(0.0f, 35.0f, 0.0f), vec3(3.10f, 1.62f, 13.8f), "desertrock3.jpg");
    auto rock_tip = add_instance(s, "rock2.obj", vec3(0.42f, 0.52f, 0.42f), vec3(0.0f, -15.0f, 0.0f), vec3(3.08f, 2.22f, 13.8f), "desertrock1.jpg");
    // Accent ground rocks around cairn base
    add_instance(s, "rock5.obj", vec3(0.55f, 0.32f, 0.50f), vec3(0.0f, 40.0f, 0.0f), vec3(2.0f, 0.15f, 13.2f), "desertrock2.jpg");
    add_instance(s, "rock1.obj", vec3(0.45f, 0.25f, 0.42f), vec3(0.0f, -20.0f, 0.0f), vec3(4.2f, 0.12f, 14.2f), "desertrock3.jpg");

    // 3. Pixar textured ball (moving_instance capable of spinning, hopping, and shutter motion blur)
    material* ball_mat = setup_material(s, "pixar_ball.png");
    ball_mat->ks = color(0.20f, 0.20f, 0.20f);
    sphere* raw_ball = new sphere(point3(0.0f, 0.0f, 0.0f), 0.60f, ball_mat);
    s.allocated_hittables.push_back(raw_ball);
    auto ball = make_shared<moving_instance>(
        raw_ball, ball_mat,
        0.0f, 0.5f,
        vec3(0.0f, 0.0f, 0.0f), vec3(0.18f, 0.04f, 0.01f),
        0.0f, 0.0f,
        vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f)
    );
    ball->frame_trans = vec3(1.7f, 0.60f, 12.2f);
    s.world.add(ball);

    // 4. Turtle with rotational motion blur (moving_instance with overwritten motion blur quantity)
    point3 turtle_pos(-0.5f, 0.35f, 13.0f);
    auto turtle = add_moving_instance(
        s, "turtle.obj",
        vec3(1.35f, 1.35f, 1.35f), // scale
        vec3(0.0f, 0.0f, 0.0f),     // base rotation
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 60.0f,                // initial rotational blur around Y
        "turtle.png"
    );
    turtle->frame_trans = turtle_pos;

    // 5. Coyote on ACME rocket closer to camera (z = 8.0) with translational motion blur
    vec3 coyote_start_pos(22.0f, 1.45f, 8.0f);
    auto coyote = add_moving_instance(
        s, "wile_e_coyote_rocket.obj",
        coyote_scale,                 // scale
        vec3(0.0f, -90.0f, 0.0f),      // 90 degrees around y
        vec3(0.0f, 0.0f, 0.0f), vec3(-1.5f, 0.0f, 0.0f), // motion blur streak along -X
        0.0f, 0.0f,
        "wile_e_coyote_rocket.png"
    );
    coyote->frame_trans = coyote_start_pos;

    // Sky texture and lighting
    environment_texture* sky_tex = new environment_texture("../models/cartoon_canyon_sky_equirectangular_4k.png", 2.2f, 0.0f, 0.26f);
    s.cam.environment_texture_sky = sky_tex;
    s.textures.push_back(sky_tex);

    vec3 sun_dir = unit_vector(vec3(0.25f, 0.80f, -0.50f));
    directional_light* dir_light = new directional_light(sun_dir, color(0.35f, 0.35f, 0.35f), color(0.95f, 0.92f, 0.85f), color(0.15f, 0.15f, 0.15f));
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);

    // Fill light from camera direction so front-facing cartoon textures are brightly illuminated
    directional_light* fill_light = new directional_light(unit_vector(vec3(0.0f, 2.0f, -1.0f)), color(0.0f, 0.0f, 0.0f), color(0.40f, 0.40f, 0.40f), color(0.05f, 0.05f, 0.05f));
    s.lights.add(fill_light);
    s.allocated_lights.push_back(fill_light);

    // 3 seconds duration (90 frames at 30 fps)
    s.wait(3.0f);

    int total_frames = ceil(s.framerate * 3.0f); // 90 frames
    for (int i = 0; i < total_frames; i++)
    {
        float f = (total_frames > 1) ? (float)i / (float)(total_frames - 1) : 0.0f;

        // 1. Coyote closer to camera at z = 8.0f, rocketing fast from right to left (+X to -X)
        // Passes turtle (x = -0.5) around frame 22 (approx 0.73s mark)
        float coyote_x = 22.0f - (float)i * 1.0f;
        InstanceTransformations trans_coyote;
        trans_coyote.inst = coyote;
        trans_coyote.setTranslation(coyote_x, 1.45f, 8.0f);
        trans_coyote.rotationVector = vec3(0.0f, 0.0f, 0.0f);
        trans_coyote.has_blur_override = true;
        trans_coyote.blur_trans = vec3(-1.5f, 0.0f, 0.0f);

        // 2. Ball kicked out to the right (+X), bouncing, spinning, and with motion blur
        float ball_x = (1.0f - f) * 1.7f + f * 24.0f;
        float ball_z = 12.2f + 1.2f * f;

        // Energetic parabolic hops mimicking tumbleweed hops but even faster
        float ball_y = 0.60f;
        float hop_p = 0.0f;
        if (f < 0.50f) {
            hop_p = f / 0.50f;
            ball_y = 0.60f + 1.80f * 4.0f * hop_p * (1.0f - hop_p);
        } else {
            hop_p = (f - 0.50f) / 0.50f;
            ball_y = 0.60f + 1.10f * 4.0f * hop_p * (1.0f - hop_p);
        }

        // Fast roll around Z axis to the right plus rapid spin around Y and wobble around X
        float rot_z = -f * 3600.0f;               // 10 full rolls forward
        float rot_y = f * 1440.0f;                // 4 full spins
        float rot_x = 25.0f * sin(f * 8.0f * pi); // rapid wobble bounce

        InstanceTransformations trans_ball;
        trans_ball.inst = ball;
        trans_ball.setTranslation(ball_x, ball_y, ball_z);
        trans_ball.rotationVector = vec3(rot_x, rot_y, rot_z);
        trans_ball.has_blur_override = true;
        // Motion blur on ball: translational streak along flight direction and rolling blur
        float ball_blur_dy = (f < 0.50f ? 0.06f : 0.03f) * (1.0f - 2.0f * hop_p);
        trans_ball.blur_trans = vec3(0.18f, ball_blur_dy, 0.01f);
        trans_ball.blur_rot = vec3(0.0f, 8.0f, -20.0f);

        // 3. Turtle animation:
        // Before Coyote passes (i < 22): upright in place with gentle rotational blur
        // When Coyote gets past (i >= 22): capsize immediately (roll to 180 deg)
        // Then animate real spinning while reducing blur, and finally make her stop
        // Motion blur is controlled by overwriting the motion quantity value on the moving instance,
        // so that when she stops, the motion blur quantity gets practically zero.
        float turtle_y = 0.35f;
        float turtle_roll = 0.0f;
        float turtle_pitch = 0.0f;
        float turtle_yaw = 0.0f;
        float turtle_blur_yaw = 0.0001f; // practically zero default

        if (i < 22) {
            // Upright before coyote passes
            turtle_y = 0.35f;
            turtle_roll = 0.0f;
            turtle_pitch = 0.0f;
            turtle_yaw = 0.0f;
            turtle_blur_yaw = 60.0f;
        } else if (i == 22) {
            // Immediate capsize snap (frame 22)
            turtle_y = 0.42f;
            turtle_roll = 90.0f;
            turtle_pitch = 5.0f;
            turtle_yaw = 30.0f;
            turtle_blur_yaw = 120.0f;
        } else {
            // Capsized upside down on shell (i >= 23)
            turtle_y = 0.50f;
            int spin_frame = i - 23;
            const int spin_duration = 52; // spins from frame 23 to 75, stops completely for 75-89

            if (spin_frame < spin_duration) {
                float u = (float)spin_frame / (float)spin_duration;
                // Cubic ease-out deceleration for real frame spinning (5 full revolutions = 1800 deg)
                float decel = 1.0f - u;
                turtle_yaw = 1800.0f * (1.0f - decel * decel * decel);
                // Wobble on curved shell while spinning
                turtle_pitch = 8.0f * decel * sin(u * 10.0f * pi);
                turtle_roll = 180.0f + 4.0f * decel * cos(u * 10.0f * pi);
                // Overwriting motion blur quantity proportional to angular velocity; reaches practically zero
                turtle_blur_yaw = 120.0f * decel * decel + 0.0001f;
            } else {
                // Finally stopped completely! Crisp and still upside-down on shell
                // Motion blur quantity overwritten to practically zero (0.0001 deg)
                turtle_yaw = 1800.0f;
                turtle_pitch = 0.0f;
                turtle_roll = 180.0f;
                turtle_blur_yaw = 0.0001f; // practically zero motion quantity value
            }
        }

        InstanceTransformations trans_turtle;
        trans_turtle.inst = turtle;
        trans_turtle.setTranslation(-0.5f, turtle_y, 13.0f);
        trans_turtle.rotationVector = vec3(turtle_pitch, turtle_yaw, turtle_roll);
        trans_turtle.has_blur_override = true;
        trans_turtle.blur_trans = vec3(0.0f, 0.0f, 0.0f);
        trans_turtle.blur_rot = vec3(0.0f, turtle_blur_yaw, 0.0f);
        trans_turtle.blur_rot_y = turtle_blur_yaw;

        std::vector<InstanceTransformations> frame_vec;
        frame_vec.push_back(trans_coyote);
        frame_vec.push_back(trans_ball);
        frame_vec.push_back(trans_turtle);
        s.frame_transforms.push_back(frame_vec);
    }

    return s;
}

inline SceneData running_coyote_scene()
{
// running scene 2. Duplicate running scene one, replace road runner with Wile E. Coyote. Duration: 3 seconds.
    SceneData s;
    
    setup_camera(s, point3(0.0f, 3.8f, 2.0f), point3(0.0f, 2.0f, 13.5f), 48.0f);
    s.cam.t0 = 0.0f;
    s.cam.t1 = 0.5f;
    s.cam.csi = 48.0f;
    s.cam.samples_per_pixel = 48; 
    s.wait(3.0f);

    // Sky environment texture
    environment_texture* sky_tex = new environment_texture("../models/cartoon_canyon_sky_equirectangular_4k.png", 2.2f, 0.0f, 0.26f);
    s.cam.environment_texture_sky = sky_tex;
    s.textures.push_back(sky_tex);

    // Directional sunlight + fill light
    vec3 sun_dir = unit_vector(vec3(0.25f, 0.80f, -0.50f));
    directional_light* dir_light = new directional_light(sun_dir, color(0.35f, 0.35f, 0.35f), color(0.95f, 0.92f, 0.85f), color(0.15f, 0.15f, 0.15f));
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);

    ambient_light = color(0.40f, 0.38f, 0.35f);
    point_light* fill_light = new point_light(point3(0.0f, 8.0f, 2.0f), color(0.20f, 0.20f, 0.20f), color(0.45f, 0.42f, 0.38f), color(0.10f, 0.10f, 0.10f));
    s.lights.add(fill_light);
    s.allocated_lights.push_back(fill_light);
    
    // Ground plane with cartoon desert texture
    texture* ground_tex = new image_texture("../models/cartoon_desert_ground.png", 6.0f, 6.0f);
    s.textures.push_back(ground_tex);
    material* ground_mat = new material();
    ground_mat->tex = ground_tex;
    ground_mat->ka = color(0.35f, 0.35f, 0.35f);
    ground_mat->kd = color(0.85f, 0.85f, 0.85f);
    ground_mat->ks = color(0.05f, 0.05f, 0.05f);
    ground_mat->is_toon = false; // Disable toon silhouette on infinite ground plane to avoid horizon blackout
    ground_mat->epsilon = 0.0f;
    s.materials.push_back(ground_mat);

    auto ground_plane = make_shared<plane>(point3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), ground_mat);
    s.world.add(ground_plane);

    // Distant background canyon formations (Z: 95-125)
    auto bgr_1 = add_instance(s, "rock4.obj", vec3(36.0f, 22.0f, 28.0f), vec3(0.0f, -25.0f, 0.0f), vec3(48.0f, 11.0f, 110.0f), "desertrock2.jpg", 2.0f, 2.0f);
    auto bgr_2 = add_instance(s, "rock2.obj", vec3(18.0f, 28.0f, 18.0f), vec3(0.0f, 35.0f, 0.0f), vec3(32.0f, 25.0f, 95.0f), "desertrock3.jpg", 1.5f, 2.0f);
    auto bgr_3 = add_instance(s, "rock4.obj", vec3(32.0f, 16.0f, 25.0f), vec3(0.0f, 10.0f, 0.0f), vec3(0.0f, 8.0f, 118.0f), "desertrock2.jpg", 2.0f, 2.0f);
    auto bgr_4 = add_instance(s, "rock4.obj", vec3(38.0f, 24.0f, 30.0f), vec3(0.0f, 30.0f, 0.0f), vec3(-50.0f, 12.0f, 112.0f), "desertrock3.jpg", 2.0f, 2.0f);
    auto bgr_5 = add_instance(s, "rock2.obj", vec3(20.0f, 30.0f, 20.0f), vec3(0.0f, -30.0f, 0.0f), vec3(-33.0f, 27.0f, 96.0f), "desertrock2.jpg", 1.5f, 2.0f);

    // Midway background elements (closer to rocks / midway: Z: 33-72) drifting with depth-based optical parallax
    struct MidwayElement {
        shared_ptr<instance> inst;
        vec3 base_pos;
    };
    std::vector<MidwayElement> midway_elements;

    // 1. Midway rock formations closer to the big rocks (Z: 65-72)
    auto mid_rock_mesa = add_instance(s, "rock4.obj", vec3(18.0f, 10.0f, 14.0f), vec3(0.0f, 15.0f, 0.0f), vec3(-16.0f, 4.5f, 72.0f), "desertrock1.jpg", 1.8f, 1.8f);
    midway_elements.push_back({ mid_rock_mesa, vec3(-16.0f, 4.5f, 72.0f) });

    auto mid_rock_spire = add_instance(s, "rock2.obj", vec3(8.0f, 14.0f, 8.0f), vec3(0.0f, -25.0f, 0.0f), vec3(14.0f, 6.5f, 65.0f), "desertrock3.jpg", 1.5f, 1.8f);
    midway_elements.push_back({ mid_rock_spire, vec3(14.0f, 6.5f, 65.0f) });

    auto mid_rock_ridge_l = add_instance(s, "rock3.obj", vec3(15.0f, 7.5f, 12.0f), vec3(0.0f, 40.0f, 0.0f), vec3(-38.0f, 3.2f, 68.0f), "desertrock2.jpg", 1.5f, 1.5f);
    midway_elements.push_back({ mid_rock_ridge_l, vec3(-38.0f, 3.2f, 68.0f) });

    auto mid_rock_ridge_r = add_instance(s, "rock4.obj", vec3(16.0f, 8.5f, 13.0f), vec3(0.0f, -30.0f, 0.0f), vec3(36.0f, 3.8f, 70.0f), "desertrock1.jpg", 1.5f, 1.5f);
    midway_elements.push_back({ mid_rock_ridge_r, vec3(36.0f, 3.8f, 70.0f) });

    // 2. Midground boulder formations and outcroppings (Z: 46-54)
    auto bgr_mid_1 = add_instance(s, "rock3.obj", vec3(7.0f, 4.5f, 6.0f), vec3(0.0f, 20.0f, 0.0f), vec3(-18.0f, 1.8f, 52.0f), "desertrock1.jpg", 1.5f, 1.5f);
    midway_elements.push_back({ bgr_mid_1, vec3(-18.0f, 1.8f, 52.0f) });

    auto bgr_mid_2 = add_instance(s, "rock5.obj", vec3(5.5f, 3.8f, 5.0f), vec3(0.0f, -35.0f, 0.0f), vec3(16.0f, 1.5f, 46.0f), "desertrock2.jpg", 1.5f, 1.5f);
    midway_elements.push_back({ bgr_mid_2, vec3(16.0f, 1.5f, 46.0f) });

    auto mid_rock_center = add_instance(s, "rock5.obj", vec3(6.5f, 3.2f, 5.5f), vec3(0.0f, 10.0f, 0.0f), vec3(-2.0f, 1.2f, 50.0f), "desertrock3.jpg", 1.5f, 1.5f);
    midway_elements.push_back({ mid_rock_center, vec3(-2.0f, 1.2f, 50.0f) });

    auto mid_rock_flank = add_instance(s, "rock1.obj", vec3(5.0f, 3.0f, 4.5f), vec3(0.0f, -15.0f, 0.0f), vec3(24.0f, 1.1f, 48.0f), "desertrock2.jpg", 1.5f, 1.5f);
    midway_elements.push_back({ mid_rock_flank, vec3(24.0f, 1.1f, 48.0f) });

    // 3. Midway saguaro cacti and desert floor clusters (Z: 33-64)
    auto mid_c_far_l = add_cactus(s, vec3(1.6f, 1.6f, 1.6f), vec3(0.0f, 25.0f, 0.0f), vec3(-24.0f, 0.0f, 64.0f));
    midway_elements.push_back({ mid_c_far_l, vec3(-24.0f, 0.0f, 64.0f) });

    auto mid_c_far_r = add_cactus(s, vec3(1.5f, 1.5f, 1.5f), vec3(0.0f, -20.0f, 0.0f), vec3(22.0f, 0.0f, 62.0f));
    midway_elements.push_back({ mid_c_far_r, vec3(22.0f, 0.0f, 62.0f) });

    auto mid_c_mid_l = add_cactus(s, vec3(1.35f, 1.35f, 1.35f), vec3(0.0f, 30.0f, 0.0f), vec3(-6.5f, 0.0f, 52.0f));
    midway_elements.push_back({ mid_c_mid_l, vec3(-6.5f, 0.0f, 52.0f) });

    auto mid_c_mid_r = add_cactus(s, vec3(1.10f, 1.10f, 1.10f), vec3(0.0f, -15.0f, 0.0f), vec3(5.5f, 0.0f, 49.0f));
    midway_elements.push_back({ mid_c_mid_r, vec3(5.5f, 0.0f, 49.0f) });

    auto mid_c_near_r1 = add_cactus(s, vec3(1.25f, 1.25f, 1.25f), vec3(0.0f, -25.0f, 0.0f), vec3(10.5f, 0.0f, 38.0f));
    midway_elements.push_back({ mid_c_near_r1, vec3(10.5f, 0.0f, 38.0f) });

    auto mid_c_near_r2 = add_cactus(s, vec3(0.90f, 0.90f, 0.90f), vec3(0.0f, 35.0f, 0.0f), vec3(12.2f, 0.0f, 39.5f));
    midway_elements.push_back({ mid_c_near_r2, vec3(12.2f, 0.0f, 39.5f) });

    auto mid_c_near_r_rock = add_instance(s, "rock1.obj", vec3(0.7f, 0.35f, 0.6f), vec3(0.0f, 15.0f, 0.0f), vec3(9.5f, 0.15f, 37.5f), "desertrock1.jpg");
    midway_elements.push_back({ mid_c_near_r_rock, vec3(9.5f, 0.15f, 37.5f) });

    auto mid_c_near_l1 = add_cactus(s, vec3(1.15f, 1.15f, 1.15f), vec3(0.0f, 20.0f, 0.0f), vec3(-11.5f, 0.0f, 34.0f));
    midway_elements.push_back({ mid_c_near_l1, vec3(-11.5f, 0.0f, 34.0f) });

    auto mid_c_near_l2 = add_cactus(s, vec3(0.85f, 0.85f, 0.85f), vec3(0.0f, -30.0f, 0.0f), vec3(-13.2f, 0.0f, 35.5f));
    midway_elements.push_back({ mid_c_near_l2, vec3(-13.2f, 0.0f, 35.5f) });

    auto mid_c_near_l_rock = add_instance(s, "rock5.obj", vec3(0.8f, 0.4f, 0.7f), vec3(0.0f, 40.0f, 0.0f), vec3(-10.2f, 0.18f, 33.2f), "desertrock3.jpg");
    midway_elements.push_back({ mid_c_near_l_rock, vec3(-10.2f, 0.18f, 33.2f) });
    
    // Catmull-Rom open spline for background rocks slow drift: dt = 0.5f / s.framerate
    float rock_dt = 0.5f / s.framerate;
    point3_list rock_spline_cps;
    rock_spline_cps.add(point3(0.0f, 0.0f, 0.0f));
    rock_spline_cps.add(point3(2.5f, 0.0f, 0.0f));
    rock_spline_cps.add(point3(5.0f, 0.0f, 0.0f));
    rock_spline_cps.add(point3(8.0f, 0.0f, 0.0f));
    path rock_path = catmull_rom_interpolate(rock_spline_cps, rock_dt, false);

    // Three staggered groups of cacti moving at moderate speed with low motion blur (moving left to right)
    // Group 1: 3 cacti + accent rock (Z ~ 18.0f)
    auto c1_a = add_moving_instance(s, "cactus.obj", vec3(1.35f, 1.35f, 1.35f), vec3(0.0f, 20.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c1_a->getMaterial()->is_toon = true;
    c1_a->getMaterial()->epsilon = 0.32f;
    c1_a->getMaterial()->K = 3;
    c1_a->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c1_a->frame_trans = vec3(-40.0f, 0.0f, 18.0f);

    auto c1_b = add_moving_instance(s, "cactus.obj", vec3(0.95f, 0.95f, 0.95f), vec3(0.0f, -35.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c1_b->getMaterial()->is_toon = true;
    c1_b->getMaterial()->epsilon = 0.32f;
    c1_b->getMaterial()->K = 3;
    c1_b->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c1_b->frame_trans = vec3(-38.8f, 0.0f, 18.6f);

    auto c1_c = add_moving_instance(s, "cactus.obj", vec3(0.70f, 0.70f, 0.70f), vec3(0.0f, 45.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c1_c->getMaterial()->is_toon = true;
    c1_c->getMaterial()->epsilon = 0.32f;
    c1_c->getMaterial()->K = 3;
    c1_c->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c1_c->frame_trans = vec3(-41.0f, 0.0f, 18.3f);

    auto c1_rock = add_moving_instance(s, "rock1.obj", vec3(0.85f, 0.55f, 0.80f), vec3(0.0f, 25.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "desertrock3.jpg");
    c1_rock->getMaterial()->is_toon = true;
    c1_rock->getMaterial()->epsilon = 0.32f;
    c1_rock->getMaterial()->K = 3;
    c1_rock->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c1_rock->frame_trans = vec3(-39.7f, 0.22f, 17.6f);

    // Group 2: Hero cactus + perched Crow from scene 1 + companion cacti (Z ~ 14.5f)
    auto c2_a = add_moving_instance(s, "cactus.obj", vec3(1.10f, 1.10f, 1.10f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c2_a->getMaterial()->is_toon = true;
    c2_a->getMaterial()->epsilon = 0.32f;
    c2_a->getMaterial()->K = 3;
    c2_a->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c2_a->frame_trans = vec3(-40.0f, 0.0f, 14.5f);

    auto crow = add_moving_instance(s, "crow.obj", vec3(0.0088f, 0.0088f, 0.0088f), vec3(0.0f, -40.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "crow.png");
    crow->getMaterial()->is_toon = true;
    crow->getMaterial()->epsilon = 0.30f;
    crow->getMaterial()->K = 3;
    crow->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    crow->frame_trans = vec3(-39.982f, 2.706f, 15.155f);

    auto c2_b = add_moving_instance(s, "cactus.obj", vec3(0.85f, 0.85f, 0.85f), vec3(0.0f, 30.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c2_b->getMaterial()->is_toon = true;
    c2_b->getMaterial()->epsilon = 0.32f;
    c2_b->getMaterial()->K = 3;
    c2_b->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c2_b->frame_trans = vec3(-38.6f, 0.0f, 15.2f);

    auto c2_c = add_moving_instance(s, "cactus.obj", vec3(0.70f, 0.70f, 0.70f), vec3(0.0f, -40.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c2_c->getMaterial()->is_toon = true;
    c2_c->getMaterial()->epsilon = 0.32f;
    c2_c->getMaterial()->K = 3;
    c2_c->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c2_c->frame_trans = vec3(-41.2f, 0.0f, 15.0f);

    // Group 3: 3 cacti (Z ~ 16.2f)
    auto c3_a = add_moving_instance(s, "cactus.obj", vec3(1.20f, 1.20f, 1.20f), vec3(0.0f, 25.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c3_a->getMaterial()->is_toon = true;
    c3_a->getMaterial()->epsilon = 0.32f;
    c3_a->getMaterial()->K = 3;
    c3_a->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c3_a->frame_trans = vec3(-40.0f, 0.0f, 16.0f);

    auto c3_b = add_moving_instance(s, "cactus.obj", vec3(0.80f, 0.80f, 0.80f), vec3(0.0f, -30.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c3_b->getMaterial()->is_toon = true;
    c3_b->getMaterial()->epsilon = 0.32f;
    c3_b->getMaterial()->K = 3;
    c3_b->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c3_b->frame_trans = vec3(-38.7f, 0.0f, 16.6f);

    auto c3_c = add_moving_instance(s, "cactus.obj", vec3(0.65f, 0.65f, 0.65f), vec3(0.0f, 40.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    c3_c->getMaterial()->is_toon = true;
    c3_c->getMaterial()->epsilon = 0.32f;
    c3_c->getMaterial()->K = 3;
    c3_c->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    c3_c->frame_trans = vec3(-41.1f, 0.0f, 16.3f);

    // Wile E. Coyote: centered in viewport, tracked by camera at high speed with concentrated motion blur
    vec3 coyote_center_pos(0.0f, 1.85f, 12.0f);
    auto coyote = add_moving_instance(
        s, "wile_e_coyote_rocket.obj",
        coyote_scale,                 // scale
        vec3(0.0f, -90.0f, 0.0f),      // 90 degrees around y
        vec3(-1.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), // concentrated motion blur centered in viewport
        0.0f, 0.0f,
        "wile_e_coyote_rocket.png"
    );
    coyote->frame_trans = coyote_center_pos;

    // Motion blurred dust cloud trailing behind Coyote on the left side of the screen (screen left = +X in camera frame)
    auto dust = add_moving_instance(
        s, "dust_cloud.obj",
        vec3(2.6f, 1.25f, 1.8f),      // horizontally stretched trailing scale
        vec3(0.0f, 0.0f, 0.0f),     // base rotation
        vec3(0.0f, 0.0f, 0.0f), vec3(2.5f, 0.0f, 0.0f), // backwards motion blur trailing to screen left (+X)
        0.0f, 0.0f,
        "dust.png",
        2.5f, 1.0f
    );
    dust->frame_trans = vec3(4.8f, 0.95f, 12.2f);
    dust->getMaterial()->is_toon = true;
    dust->getMaterial()->epsilon = 0.28f;
    dust->getMaterial()->K = 3;
    dust->getMaterial()->ka = color(0.36f, 0.21f, 0.10f);
    dust->getMaterial()->kd = color(0.87f, 0.50f, 0.22f);
    dust->getMaterial()->ks = color(0.0f, 0.0f, 0.0f);

    // Foreground blazing fast cacti group directly obstructing camera view (Z ~ 5.2f)
    auto fg_c_a = add_moving_instance(s, "cactus.obj", vec3(2.5f, 2.5f, 2.5f), vec3(0.0f, 15.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    fg_c_a->getMaterial()->is_toon = true;
    fg_c_a->getMaterial()->epsilon = 0.35f;
    fg_c_a->getMaterial()->K = 3;
    fg_c_a->getMaterial()->ka = color(0.20f, 0.20f, 0.20f);
    fg_c_a->frame_trans = vec3(-35.0f, 0.0f, 5.2f);

    auto fg_c_b = add_moving_instance(s, "cactus.obj", vec3(1.9f, 1.9f, 1.9f), vec3(0.0f, -25.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    fg_c_b->getMaterial()->is_toon = true;
    fg_c_b->getMaterial()->epsilon = 0.35f;
    fg_c_b->getMaterial()->K = 3;
    fg_c_b->getMaterial()->ka = color(0.20f, 0.20f, 0.20f);
    fg_c_b->frame_trans = vec3(-33.8f, 0.0f, 5.5f);

    auto fg_c_c = add_moving_instance(s, "cactus.obj", vec3(1.6f, 1.6f, 1.6f), vec3(0.0f, 40.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, "cactus.png");
    fg_c_c->getMaterial()->is_toon = true;
    fg_c_c->getMaterial()->epsilon = 0.35f;
    fg_c_c->getMaterial()->K = 3;
    fg_c_c->getMaterial()->ka = color(0.20f, 0.20f, 0.20f);
    fg_c_c->frame_trans = vec3(-36.2f, 0.0f, 5.0f);

    // Catmull-Rom open spline with coarsest density (dt = 1.0f / 3.0f, only 3 steps per segment)
    point3_list fg_spline_cps;
    fg_spline_cps.add(point3(-16.0f, 0.0f, 5.2f));
    fg_spline_cps.add(point3(-6.0f, 0.0f, 5.2f));
    fg_spline_cps.add(point3(6.0f, 0.0f, 5.2f));
    fg_spline_cps.add(point3(16.0f, 0.0f, 5.2f));
    float fg_dt = 1.0f / 3.0f; // coarsest spline density
    path fg_path = catmull_rom_interpolate(fg_spline_cps, fg_dt, false);

    // Frame animations for background rocks drift and cacti crossing
    int total_frames = s.camera_positions.length();
    for (int i = 0; i < total_frames; i++)
    {
        point3 rock_offset = (i < rock_path.points.size()) ? rock_path.points[i] : rock_path.points.back();

        // 1. Five distant background rocks drifting slowly with Catmull-Rom spline (zero motion blur)
        InstanceTransformations trans_r1;
        trans_r1.inst = bgr_1;
        trans_r1.setTranslation(48.0f + rock_offset.x(), 11.0f, 110.0f);
        trans_r1.has_blur_override = false;

        InstanceTransformations trans_r2;
        trans_r2.inst = bgr_2;
        trans_r2.setTranslation(32.0f + rock_offset.x(), 25.0f, 95.0f);
        trans_r2.has_blur_override = false;

        InstanceTransformations trans_r3;
        trans_r3.inst = bgr_3;
        trans_r3.setTranslation(0.0f + rock_offset.x(), 8.0f, 118.0f);
        trans_r3.has_blur_override = false;

        InstanceTransformations trans_r4;
        trans_r4.inst = bgr_4;
        trans_r4.setTranslation(-50.0f + rock_offset.x(), 12.0f, 112.0f);
        trans_r4.has_blur_override = false;

        InstanceTransformations trans_r5;
        trans_r5.inst = bgr_5;
        trans_r5.setTranslation(-33.0f + rock_offset.x(), 27.0f, 96.0f);
        trans_r5.has_blur_override = false;

        // 2. Midway background elements (closer to rocks / midway: Z: 33-72) drifting with optical parallax (zero motion blur)
        std::vector<InstanceTransformations> trans_midway;
        for (const auto& elem : midway_elements) {
            float p = 98.0f / (elem.base_pos.z() - 2.0f);
            float drift = rock_offset.x() * p;
            InstanceTransformations t;
            t.inst = elem.inst;
            t.setTranslation(elem.base_pos.x() + drift, elem.base_pos.y(), elem.base_pos.z());
            t.has_blur_override = false;
            trans_midway.push_back(t);
        }

        // 3. Group 1 cacti + accent rock: crossing left to right across frames 0..44 with low motion blur
        float g1_x = -40.0f;
        vec3 g1_blur(0.0f, 0.0f, 0.0f);
        if (i <= 44) {
            float u = (float)i / 44.0f;
            g1_x = (1.0f - u) * (-18.0f) + u * 18.0f;
            g1_blur = vec3(0.50f, 0.0f, 0.0f);
        } else {
            g1_x = 40.0f;
            g1_blur = vec3(0.0f, 0.0f, 0.0f);
        }

        InstanceTransformations trans_c1_a;
        trans_c1_a.inst = c1_a;
        trans_c1_a.setTranslation(g1_x, 0.0f, 18.0f);
        trans_c1_a.has_blur_override = true;
        trans_c1_a.blur_trans = g1_blur;

        InstanceTransformations trans_c1_b;
        trans_c1_b.inst = c1_b;
        trans_c1_b.setTranslation(g1_x + 1.2f, 0.0f, 18.6f);
        trans_c1_b.has_blur_override = true;
        trans_c1_b.blur_trans = g1_blur;

        InstanceTransformations trans_c1_c;
        trans_c1_c.inst = c1_c;
        trans_c1_c.setTranslation(g1_x - 1.0f, 0.0f, 18.3f);
        trans_c1_c.has_blur_override = true;
        trans_c1_c.blur_trans = g1_blur;

        InstanceTransformations trans_c1_rock;
        trans_c1_rock.inst = c1_rock;
        trans_c1_rock.setTranslation(g1_x + 0.3f, 0.22f, 17.6f);
        trans_c1_rock.has_blur_override = true;
        trans_c1_rock.blur_trans = g1_blur;

        // 4. Group 2 cacti + Crow: crossing left to right across frames 22..68 with low motion blur
        float g2_x = -40.0f;
        vec3 g2_blur(0.0f, 0.0f, 0.0f);
        if (i < 22) {
            g2_x = -40.0f;
            g2_blur = vec3(0.0f, 0.0f, 0.0f);
        } else if (i <= 68) {
            float u = (float)(i - 22) / 46.0f;
            g2_x = (1.0f - u) * (-18.0f) + u * 18.0f;
            g2_blur = vec3(0.55f, 0.0f, 0.0f);
        } else {
            g2_x = 40.0f;
            g2_blur = vec3(0.0f, 0.0f, 0.0f);
        }

        InstanceTransformations trans_c2_a;
        trans_c2_a.inst = c2_a;
        trans_c2_a.setTranslation(g2_x, 0.0f, 14.5f);
        trans_c2_a.has_blur_override = true;
        trans_c2_a.blur_trans = g2_blur;

        InstanceTransformations trans_crow;
        trans_crow.inst = crow;
        trans_crow.setTranslation(g2_x + 0.018f, 2.706f, 15.155f);
        trans_crow.has_blur_override = true;
        trans_crow.blur_trans = g2_blur;

        InstanceTransformations trans_c2_b;
        trans_c2_b.inst = c2_b;
        trans_c2_b.setTranslation(g2_x + 1.4f, 0.0f, 15.2f);
        trans_c2_b.has_blur_override = true;
        trans_c2_b.blur_trans = g2_blur;

        InstanceTransformations trans_c2_c;
        trans_c2_c.inst = c2_c;
        trans_c2_c.setTranslation(g2_x - 1.2f, 0.0f, 15.0f);
        trans_c2_c.has_blur_override = true;
        trans_c2_c.blur_trans = g2_blur;

        // 5. Group 3 cacti: crossing left to right across frames 44..89 with low motion blur
        float g3_x = -40.0f;
        vec3 g3_blur(0.0f, 0.0f, 0.0f);
        if (i < 44) {
            g3_x = -40.0f;
            g3_blur = vec3(0.0f, 0.0f, 0.0f);
        } else {
            float u = (float)(i - 44) / 45.0f;
            g3_x = (1.0f - u) * (-18.0f) + u * 18.0f;
            g3_blur = vec3(0.55f, 0.0f, 0.0f);
        }

        InstanceTransformations trans_c3_a;
        trans_c3_a.inst = c3_a;
        trans_c3_a.setTranslation(g3_x, 0.0f, 16.0f);
        trans_c3_a.has_blur_override = true;
        trans_c3_a.blur_trans = g3_blur;

        InstanceTransformations trans_c3_b;
        trans_c3_b.inst = c3_b;
        trans_c3_b.setTranslation(g3_x + 1.3f, 0.0f, 16.6f);
        trans_c3_b.has_blur_override = true;
        trans_c3_b.blur_trans = g3_blur;

        InstanceTransformations trans_c3_c;
        trans_c3_c.inst = c3_c;
        trans_c3_c.setTranslation(g3_x - 1.1f, 0.0f, 16.3f);
        trans_c3_c.has_blur_override = true;
        trans_c3_c.blur_trans = g3_blur;

        // 6. Foreground blazing fast cacti directly obstructing camera view (crossing during frames 36..44)
        float fg_x = -35.0f;
        vec3 fg_blur(0.0f, 0.0f, 0.0f);
        if (i < 36) {
            fg_x = -35.0f;
            fg_blur = vec3(0.0f, 0.0f, 0.0f);
        } else if (i <= 44) {
            int step_idx = i - 36;
            if (step_idx < (int)fg_path.points.size()) {
                fg_x = fg_path.points[step_idx].x();
            } else {
                fg_x = 18.0f;
            }
            fg_blur = vec3(5.5f, 0.0f, 0.0f);
        } else {
            fg_x = 35.0f;
            fg_blur = vec3(0.0f, 0.0f, 0.0f);
        }

        InstanceTransformations trans_fg_a;
        trans_fg_a.inst = fg_c_a;
        trans_fg_a.setTranslation(fg_x, 0.0f, 5.2f);
        trans_fg_a.has_blur_override = true;
        trans_fg_a.blur_trans = fg_blur;

        InstanceTransformations trans_fg_b;
        trans_fg_b.inst = fg_c_b;
        trans_fg_b.setTranslation(fg_x + 1.2f, 0.0f, 5.5f);
        trans_fg_b.has_blur_override = true;
        trans_fg_b.blur_trans = fg_blur;

        InstanceTransformations trans_fg_c;
        trans_fg_c.inst = fg_c_c;
        trans_fg_c.setTranslation(fg_x - 1.1f, 0.0f, 5.0f);
        trans_fg_c.has_blur_override = true;
        trans_fg_c.blur_trans = fg_blur;

        // 7. Coyote (centered)
        InstanceTransformations trans_coyote;
        trans_coyote.inst = coyote;
        trans_coyote.setTranslation(coyote_center_pos.x(), coyote_center_pos.y(), coyote_center_pos.z());
        trans_coyote.rotationVector = vec3(0.0f, 0.0f, 0.0f);

        // 8. Dust cloud trailing behind Coyote on the left side (screen left = +X)
        InstanceTransformations trans_dust;
        trans_dust.inst = dust;
        trans_dust.setTranslation(dust->frame_trans.x(), dust->frame_trans.y(), dust->frame_trans.z());
        trans_dust.has_blur_override = true;
        trans_dust.blur_trans = vec3(2.5f, 0.0f, 0.0f);

        std::vector<InstanceTransformations> frame_vec;
        frame_vec.push_back(trans_r1);
        frame_vec.push_back(trans_r2);
        frame_vec.push_back(trans_r3);
        frame_vec.push_back(trans_r4);
        frame_vec.push_back(trans_r5);
        for (const auto& t : trans_midway) {
            frame_vec.push_back(t);
        }
        frame_vec.push_back(trans_c1_a);
        frame_vec.push_back(trans_c1_b);
        frame_vec.push_back(trans_c1_c);
        frame_vec.push_back(trans_c1_rock);
        frame_vec.push_back(trans_c2_a);
        frame_vec.push_back(trans_crow);
        frame_vec.push_back(trans_c2_b);
        frame_vec.push_back(trans_c2_c);
        frame_vec.push_back(trans_c3_a);
        frame_vec.push_back(trans_c3_b);
        frame_vec.push_back(trans_c3_c);
        frame_vec.push_back(trans_fg_a);
        frame_vec.push_back(trans_fg_b);
        frame_vec.push_back(trans_fg_c);
        frame_vec.push_back(trans_coyote);
        frame_vec.push_back(trans_dust);

        s.frame_transforms.push_back(frame_vec);
    }
    
    return s;
}

inline SceneData coyote_time_scene()
{
    // Falling scene (coyote time):
    // - Roadrunner dashes in and stops right at the cliff edge.
    // - Coyote rockets in behind, overshoots past the edge into open air.
    // - The camera tracks right to follow Coyote out into mid-air.
    // - Both stop in mid-air for 3 seconds (90 frames of frozen suspense).
    // - Coyote drops vertically out of field with high downward motion blur.
    // Duration: 10 seconds (300 frames at 30 fps).
    SceneData s;

    // Camera setup with shutter motion blur ([t0, t1] = [0.0, 0.5], 24 shutter instants)
    setup_camera(s, point3(0.0f, 3.8f, 2.0f), point3(2.5f, 2.0f, 12.0f), 48.0f);
    s.cam.t0 = 0.0f;
    s.cam.t1 = 0.5f;
    s.cam.csi = 24.0f;
    s.cam.samples_per_pixel = 24;
    s.move_also_lookat = true;

    // Sky environment texture
    environment_texture* sky_tex = new environment_texture("../models/cartoon_canyon_sky_equirectangular_4k.png", 2.2f, 0.0f, 0.26f);
    s.cam.environment_texture_sky = sky_tex;
    s.textures.push_back(sky_tex);

    // Directional sunlight + fill light
    vec3 sun_dir = unit_vector(vec3(0.25f, 0.80f, -0.50f));
    directional_light* dir_light = new directional_light(sun_dir, color(0.35f, 0.35f, 0.35f), color(0.95f, 0.92f, 0.85f), color(0.15f, 0.15f, 0.15f));
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);

    directional_light* fill_light = new directional_light(unit_vector(vec3(0.0f, 2.0f, -1.0f)), color(0.0f, 0.0f, 0.0f), color(0.40f, 0.40f, 0.40f), color(0.05f, 0.05f, 0.05f));
    s.lights.add(fill_light);
    s.allocated_lights.push_back(fill_light);

    // Rocky cliff ground promontory:
    // Natural canyon rock slabs and pillars forming the ground and sheer cliff edge.
    // The background canyon landscape is provided by the equirectangular sky texture.

    // 1. Hero cliff edge rock slab (jutting promontory ending at x = 0.0f)
    auto cliff_rock = add_instance(s, "rock3.obj", vec3(3.8f, 1.4f, 3.8f), vec3(0.0f, 25.0f, 0.0f), vec3(-0.4f, -0.6f, 12.0f), "desertrock1.jpg");
    cliff_rock->getMaterial()->kd = color(0.72f, 0.48f, 0.32f);

    // 2. Sheer vertical cliff pillar supporting the ledge from below
    auto cliff_pillar = add_instance(s, "rock2.obj", vec3(3.0f, 4.5f, 3.0f), vec3(0.0f, 10.0f, 0.0f), vec3(-0.9f, -3.5f, 12.0f), "desertrock1.jpg");
    cliff_pillar->getMaterial()->kd = color(0.68f, 0.44f, 0.30f);

    // 3. Main plateau rock slab extending leftward
    auto r4_plat = add_instance(s, "rock4.obj", vec3(5.0f, 1.3f, 4.2f), vec3(0.0f, -20.0f, 0.0f), vec3(-4.2f, -0.8f, 12.2f), "desertrock2.jpg");
    r4_plat->getMaterial()->kd = color(0.70f, 0.46f, 0.32f);

    // 4. Foreground flanking rock slab (closer to camera)
    auto r5_fg = add_instance(s, "rock5.obj", vec3(4.0f, 1.4f, 3.5f), vec3(0.0f, 40.0f, 0.0f), vec3(-2.6f, -0.7f, 10.4f), "desertrock3.jpg");
    r5_fg->getMaterial()->kd = color(0.74f, 0.50f, 0.34f);

    // 5. Background flanking rock slab
    auto r3_bg = add_instance(s, "rock3.obj", vec3(4.5f, 1.4f, 4.0f), vec3(0.0f, 65.0f, 0.0f), vec3(-3.5f, -0.6f, 14.2f), "desertrock1.jpg");
    r3_bg->getMaterial()->kd = color(0.68f, 0.44f, 0.30f);

    // 6. Extended rocky terrain to the left (run-in path for Roadrunner and Coyote)
    auto r4_ext1 = add_instance(s, "rock4.obj", vec3(6.0f, 1.4f, 5.0f), vec3(0.0f, 15.0f, 0.0f), vec3(-9.0f, -0.85f, 12.0f), "desertrock2.jpg");
    r4_ext1->getMaterial()->kd = color(0.70f, 0.46f, 0.32f);

    auto r3_ext2 = add_instance(s, "rock3.obj", vec3(6.5f, 1.4f, 5.5f), vec3(0.0f, -35.0f, 0.0f), vec3(-15.0f, -0.65f, 12.0f), "desertrock1.jpg");
    r3_ext2->getMaterial()->kd = color(0.72f, 0.48f, 0.32f);

    auto r4_ext3 = add_instance(s, "rock4.obj", vec3(7.5f, 1.4f, 6.0f), vec3(0.0f, 45.0f, 0.0f), vec3(-22.0f, -0.85f, 12.0f), "desertrock3.jpg");
    r4_ext3->getMaterial()->kd = color(0.68f, 0.44f, 0.30f);

    // 7. Small accent rocks on the plateau surface
    auto r1_acc1 = add_instance(s, "rock1.obj", vec3(0.8f, 0.45f, 0.7f), vec3(0.0f, 15.0f, 0.0f), vec3(-1.6f, 0.15f, 13.0f), "desertrock2.jpg");
    r1_acc1->getMaterial()->kd = color(0.72f, 0.48f, 0.32f);

    auto r5_acc2 = add_instance(s, "rock5.obj", vec3(0.7f, 0.40f, 0.6f), vec3(0.0f, -20.0f, 0.0f), vec3(-2.2f, 0.18f, 11.2f), "desertrock3.jpg");
    r5_acc2->getMaterial()->kd = color(0.70f, 0.46f, 0.32f);

    // Roadrunner moving instance (running left to right along +X)
    auto roadrunner = add_moving_instance(
        s, "roadrunner_running.obj",
        vec3(0.045f, 0.045f, 0.045f), // scale
        vec3(0.0f, 90.0f, 0.0f),      // pointed along +X
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "roadrunner.png"
    );
    roadrunner->getMaterial()->is_toon = true;
    roadrunner->getMaterial()->epsilon = 0.30f;
    roadrunner->getMaterial()->K = 3;
    roadrunner->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    roadrunner->frame_trans = vec3(-18.0f, 1.82f, 12.2f);

    // Coyote strapped to ACME rocket (running left to right along +X)
    auto coyote = add_moving_instance(
        s, "wile_e_coyote_rocket.obj",
        coyote_scale,                 // scale
        vec3(0.0f, 90.0f, 0.0f),      // pointed along +X
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "wile_e_coyote_rocket.png"
    );
    coyote->getMaterial()->is_toon = true;
    coyote->getMaterial()->epsilon = 0.30f;
    coyote->getMaterial()->K = 3;
    coyote->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    coyote->frame_trans = vec3(-26.0f, 1.85f, 11.8f);

    // 10 seconds duration (300 frames at 30 fps)
    int total_frames = ceil(s.framerate * 10.0f);
    for (int i = 0; i < total_frames; i++)
    {
        // 1. Camera horizontal tracking: follows Coyote out over the abyss
        float cam_x = 0.0f;
        if (i >= 75 && i < 130) {
            float u = (float)(i - 75) / 55.0f;
            float su = u * u * (3.0f - 2.0f * u);
            cam_x = su * 3.5f;
        } else if (i >= 130) {
            cam_x = 3.5f;
        }
        s.camera_positions.add(point3(cam_x, 3.8f, 2.0f));

        // 2. Roadrunner: rushes in from off-screen left and skids to a stop at edge (x = -0.8f)
        float rr_x = -0.8f;
        vec3 rr_blur(0.0f, 0.0f, 0.0f);
        if (i < 35) {
            float u = (float)i / 35.0f;
            float p = 1.0f - (1.0f - u) * (1.0f - u);
            rr_x = (1.0f - p) * (-18.0f) + p * (-0.8f);
            rr_blur = vec3((1.0f - u) * 1.8f, 0.0f, 0.0f);
        }
        InstanceTransformations trans_rr;
        trans_rr.inst = roadrunner;
        trans_rr.setTranslation(rr_x, 1.82f, 12.2f);
        trans_rr.rotationVector = vec3(0.0f, 0.0f, 0.0f);
        trans_rr.has_blur_override = true;
        trans_rr.blur_trans = rr_blur;

        // 3. Coyote: rushes from off-screen left, overshoots past edge to x = 5.5f,
        // stops for 3 seconds (90 frames: 130-219), then falls with vertical motion blur
        float coyote_x = -26.0f;
        float coyote_y = 1.85f;
        vec3 coyote_blur(0.0f, 0.0f, 0.0f);
        float pitch = 0.0f;

        if (i < 40) {
            coyote_x = -26.0f;
            coyote_y = 1.85f;
            coyote_blur = vec3(0.0f, 0.0f, 0.0f);
        } else if (i < 130) {
            float u = (float)(i - 40) / 90.0f;
            float p = 1.0f - pow(1.0f - u, 2.2f);
            coyote_x = (1.0f - p) * (-26.0f) + p * 5.5f;
            coyote_y = 1.85f;
            coyote_blur = vec3((1.0f - u) * 2.2f, 0.0f, 0.0f);
        } else if (i < 220) {
            // Suspended in mid-air over abyss for 3 full seconds (90 frames: 130-219)
            coyote_x = 5.5f;
            coyote_y = 1.85f;
            coyote_blur = vec3(0.0f, 0.0f, 0.0f);
            if (i >= 180 && i < 210) {
                pitch = 5.0f * sin((float)(i - 180) / 30.0f * 2.0f * pi);
            }
        } else {
            // Free-fall plunge down out of field
            coyote_x = 5.5f;
            float fall_t = (float)(i - 220) / 30.0f;
            float dy = 0.5f * 36.0f * fall_t * fall_t;
            coyote_y = 1.85f - dy;
            float speed_y = 36.0f * fall_t;
            float blur_dy = -std::min(10.0f, speed_y * (0.5f / 30.0f) * 2.5f);
            coyote_blur = vec3(0.0f, blur_dy, 0.0f);
            pitch = std::min(75.0f, fall_t * 60.0f);
        }

        InstanceTransformations trans_coyote;
        trans_coyote.inst = coyote;
        trans_coyote.setTranslation(coyote_x, coyote_y, 11.8f);
        trans_coyote.rotationVector = vec3(pitch, 0.0f, 0.0f);
        trans_coyote.has_blur_override = true;
        trans_coyote.blur_trans = coyote_blur;

        std::vector<InstanceTransformations> frame_vec;
        frame_vec.push_back(trans_rr);
        frame_vec.push_back(trans_coyote);
        s.frame_transforms.push_back(frame_vec);
    }

    return s;
}

inline SceneData final_falling_coyote_scene()
{
    // Final scene:
    // - Camera looking straight down in -Y direction from high above the canyon.
    // - Underneath, a big plane with unrepeated Wile E. Coyote cartoon canyon valley texture.
    // - Two rocks on the left and right forming towering, narrowing canyon walls.
    // - Coyote strapped to rocket falls past camera down into the gorge.
    // - Rocket exhaust directly facing camera (rocket pointed down).
    // - No motion blur, purely quadratic acceleration falling under gravity.
    // Duration: 3.5 seconds (105 frames at 30 fps).
    SceneData s;

    // Top-down camera pointing down -Y (vup pointing -Z so +X is right, -Z is up in frame)
    setup_camera(s, point3(0.0f, 26.0f, 0.0f), point3(0.0f, 0.0f, 0.0f), 52.0f);
    s.cam.vup = vec3(0.0f, 0.0f, -1.0f);
    s.cam.t0 = 0.0f;
    s.cam.t1 = 0.0f;
    s.cam.samples_per_pixel = 48;
    s.cam.csi = 0.0f;
    s.wait(3.5f);

    // Directional sunlight shining down into the gorge + fill light
    vec3 sun_dir = unit_vector(vec3(0.35f, 0.85f, 0.35f));
    directional_light* dir_light = new directional_light(sun_dir, color(0.35f, 0.35f, 0.35f), color(0.95f, 0.92f, 0.85f), color(0.15f, 0.15f, 0.15f));
    s.lights.add(dir_light);
    s.allocated_lights.push_back(dir_light);

    directional_light* fill_light = new directional_light(vec3(0.0f, 1.0f, 0.0f), color(0.15f, 0.15f, 0.15f), color(0.45f, 0.42f, 0.38f), color(0.08f, 0.08f, 0.08f));
    s.lights.add(fill_light);
    s.allocated_lights.push_back(fill_light);

    // Canyon valley floor texture
    texture* valley_tex = new image_texture("../models/canyon_valley_floor.jpg", 1.0f, 1.0f);
    s.textures.push_back(valley_tex);

    material* floor_mat = new material();
    floor_mat->tex = valley_tex;
    floor_mat->ka = color(0.40f, 0.38f, 0.35f);
    floor_mat->kd = color(0.90f, 0.88f, 0.85f);
    floor_mat->ks = color(0.02f, 0.02f, 0.02f);
    floor_mat->is_toon = false;
    s.materials.push_back(floor_mat);

    // Big plane underneath with non-repeating canyon valley texture (1:1 mapping, no tiling)
    auto canyon_floor = make_shared<quad>(point3(-60.0f, -40.0f, 34.0f), vec3(120.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -68.0f), floor_mat);
    s.world.add(canyon_floor);

    auto backup_plane = make_shared<plane>(point3(0.0f, -40.2f, 0.0f), vec3(0.0f, 1.0f, 0.0f), -60.0f, 60.0f, 34.0f, -34.0f, floor_mat);
    s.world.add(backup_plane);

    // Left canyon wall (upper rim + lower stepped cliff narrowing toward center)
    auto rock_left_upper = add_instance(s, "rock4.obj", vec3(14.0f, 26.0f, 32.0f), vec3(0.0f, 25.0f, 8.0f), vec3(-30.0f, 2.0f, 0.0f), "desertrock1.jpg");
    rock_left_upper->getMaterial()->kd = color(0.72f, 0.46f, 0.30f);

    auto rock_left_lower = add_instance(s, "rock3.obj", vec3(10.0f, 20.0f, 24.0f), vec3(0.0f, -15.0f, -10.0f), vec3(-20.0f, -18.0f, 0.0f), "desertrock2.jpg");
    rock_left_lower->getMaterial()->kd = color(0.70f, 0.44f, 0.28f);

    // Right canyon wall (upper rim + lower stepped cliff narrowing toward center)
    auto rock_right_upper = add_instance(s, "rock4.obj", vec3(14.0f, 26.0f, 32.0f), vec3(0.0f, -20.0f, -8.0f), vec3(30.0f, 2.0f, 0.0f), "desertrock1.jpg");
    rock_right_upper->getMaterial()->kd = color(0.72f, 0.46f, 0.30f);

    auto rock_right_lower = add_instance(s, "rock3.obj", vec3(10.0f, 20.0f, 24.0f), vec3(0.0f, 35.0f, 10.0f), vec3(20.0f, -18.0f, 0.0f), "desertrock3.jpg");
    rock_right_lower->getMaterial()->kd = color(0.68f, 0.42f, 0.26f);

    // Coyote strapped to rocket:
    // Base rotation vec3(90.0f, 0.0f, 0.0f) tilts nose straight down (-Y) and
    // rocket exhaust circular nozzle straight up (+Y), directly facing camera.
    auto coyote = add_moving_instance(
        s, "wile_e_coyote_rocket.obj",
        coyote_scale,                 // scale
        vec3(90.0f, 180.0f, 0.0f),    // rocket pointed down, exhaust facing camera, 180 deg around z
        vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        "wile_e_coyote_rocket.png"
    );
    coyote->getMaterial()->is_toon = true;
    coyote->getMaterial()->epsilon = 0.30f;
    coyote->getMaterial()->K = 3;
    coyote->getMaterial()->ka = color(0.25f, 0.25f, 0.25f);
    coyote->frame_trans = vec3(0.0f, 22.0f, 0.0f);

    // Multi-frame animation: 3.5 seconds = 105 frames
    int total_frames = ceil(s.framerate * 3.5f);
    for (int i = 0; i < total_frames; i++)
    {
        float u = (total_frames > 1) ? (float)i / (float)(total_frames - 1) : 0.0f;

        // Quadratic falling under gravity: y(u) = y0 - dy * u^2
        float y = 22.0f - 60.5f * (u * u);

        // Cartoon size scaling: shrinks quadratically as Coyote plunges into distance
        float scale_val = coyote_scale.x() * (1.0f - 0.50f * (u * u));

        // Comical subtle wobbling as he plummets down
        float pitch = 90.0f + 2.0f * sin(u * 14.0f * pi);
        float yaw = 3.0f * cos(u * 10.0f * pi);
        float roll = 4.0f * sin(u * 16.0f * pi);

        InstanceTransformations trans_coyote;
        trans_coyote.inst = coyote;
        trans_coyote.setTranslation(0.0f, y, 0.0f);
        trans_coyote.rotationVector = vec3(pitch, yaw, roll);
        trans_coyote.has_blur_override = false;
        trans_coyote.blur_trans = vec3(0.0f, 0.0f, 0.0f);
        trans_coyote.setScale(scale_val);

        std::vector<InstanceTransformations> frame_vec;
        frame_vec.push_back(trans_coyote);
        s.frame_transforms.push_back(frame_vec);
    }

    return s;
}



