#include <string>
#include "scenes.h"
#include "spline.h"
#include "point3_list.h"

color lightgray(0.75f, 0.75f, 0.75f);
color ambient_light(0.25f, 0.25f, 0.25f);


//Helper functions

inline void setup_camera(SceneData& s, point3 lookfrom = point3(0.0f, 0.0f, 0.0f), point3 lookat = point3(0.0f, 0.0f, 1.0f), float fov = 60.0f)
{
    s.cam.aspect_ratio = 16.0f / 9.0f;
    s.cam.image_width = 960;
    s.cam.samples_per_pixel = 1;
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

    // Tumbleweed
    auto tumbleweed = add_instance(s, "PUSHILIN_Tumbleweed.obj", vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(16.0f, 1.0f, 14.5f), "PUSHILIN_Tumbleweed.png");

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

    // Roll tumbleweed around Z axis forward (delta X = 32 -> ~1833 deg) plus subtle wobble
    int total_pts = tumbleweed_path.points.size();
    for (int i = 0; i < total_pts; i++)
    {
        point3 p = tumbleweed_path.points[i];
        float f = (total_pts > 1) ? (float)i / (float)(total_pts - 1) : 0.0f;
        float rot_z = f * 1833.5f;
        float rot_y = 25.0f * sin(f * 4.0f * pi);
        float rot_x = 15.0f * sin(f * 3.0f * pi);

        InstanceTransformations transform;
        transform.inst = tumbleweed;
        transform.setTranslation(p.x(), p.y(), p.z());
        transform.rotationVector = vec3(rot_x, rot_y, rot_z);
        s.transforms.push_back(transform);
    }

    // Pitch camera slightly down (lookat y = 4.0 at z = 15.0) to raise the horizon line to ~57% of viewport
    setup_camera(s, point3(0.0f, 5.0f, 0.0f), point3(0.0f, 4.0f, 15.0f), 50.0f);

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
// todo: a kind of still life - a cylinder with wooden texture (a wooden pole) on the left. A group of three cacti and some rocks. A turtle is slowly walking, trying to push a pixar textured ball.
// suddenly, roadrunner passes by (really fast, high motion blur). Duration: 7 seconds.
}

inline SceneData turtle_scene_coyote()
{
// todo: same still life from before - rocks stacked up one upon another, turtle has motion blur, spinning around. Suddenly, coyote passes: fast, slower than roadrunner, high motion blur. Duration: 5 seconds.
}

inline SceneData wasted_turtle_scene()
{
// todo: same still life from before - turtle is no longer spinning, but it has capsized. The pole has fallen to the ground and everything is now black (burnt). Duration: 3 seconds.
}

inline SceneData running_roadrunner_scene()
{
// todo: running scene 1. Parallax effect of cacti moving from right to left, high motion blur, very fast. On the background, scaled up great rocks moving slower (no motion blur). In the foreground, still roadrunner, with great blur effect from left to center. Duration: 3 seconds
}

inline SceneData freezed_roadrunner_scene()
{
// todo: still scene 1. Same scene from before, except everything is still. In the foreground, a duplicated roadrunner: one with motion blur, and one still, in the center. Duration: 2 seconds.
}

inline SceneData running_coyote_scene()
{
// todo: running scene 2. Duplicate running scene one, replace road runner with Wile E. Coyote. Duration: 3 seconds.
}

inline SceneData freezed_coyote_scene()
{
// todo: still scene 2. Duplicate scene 1, but replace roadrunner with Wile. Duration: 2 seconds.
}

inline SceneData coyote_time_scene()
{
// todo: falling scene - roadrunner stops at the edge. Some time later, Coyote gets past beyond the edge. The camera start following him. Then both stops for 3 seconds. Motion blur from current position to the bottom (out of field). Duration: 10 seconds.
}

inline SceneData final_falling_coyote_scene()
{
// todo: final scene - camera is looking on -y direction. In the background, a plane with canyon valley texture. Coyote falls down (gets past the camera, and gets further and further down and scaling down its size quadratically)
}



