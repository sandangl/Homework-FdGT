#include <string>
#include "scenes.h"
#include "spline.h"
#include "point3_list.h"

color lightgray(0.75f, 0.75f, 0.75f);
color ambient_light(0.25f, 0.25f, 0.25f);


//Helper functions

inline void setup_camera(SceneData& s, point3 lookfrom, point3 lookat, float fov = 60.0f)
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

inline material* setup_material(SceneData& s, const std::string& texFilename = "")
{
    material* mat = new material();
    mat->ka = lightgray;
    mat->kd = lightgray;
    mat->ks = lightgray;
    mat->is_toon = true;
    
    if (!texFilename.empty()) {
        std::string texPath = "../models/" + texFilename;
        texture* tex = new image_texture(texPath.c_str());
        mat->tex = tex;
        s.textures.push_back(tex);
    }
    
    s.materials.push_back(mat);
    
    return mat;
}

inline shared_ptr<instance> add_instance(SceneData& s, const std::string& objFilename, vec3 scaleVector, vec3 rotVector, vec3 transVector, const std::string& texFilename = "") {
    std::string objPath = "../models/" + objFilename;
    mesh* obj = new mesh(objPath.c_str(), "../models/");
    if (!obj->loaded) {
        std::cerr << "Impossibile caricare " << objPath << std::endl;
    }
    s.meshes.push_back(obj);

    material* mat = setup_material(s, texFilename);

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

// Scenes from script

inline SceneData initial_scene()
{
// camera pointing to the sky, a sphere with moon texture, an airplane passing by. Duration: 3 seconds;
    SceneData s;
    
    setup_camera(s, point3(0.0f, 0.0f, 0.0f), point3(0.0f, 1.0f, -2.0f), 40);
    setup_sky(s);
    s.wait(3.0f);
    
    auto plane = add_instance(s, "f-16.obj", vec3(0.5f, 0.5f, 0.5f), vec3(0.0f, 90.0f, 0.0f), vec3(-20.0f, 10.0f, -20.0f));
    
    point3_list plane_control_points;
    plane_control_points.add(point3(-20.0f, 10.0f, -20.0f));
    plane_control_points.add(point3(-6.67f, 10.0f, -20.0f));
    plane_control_points.add(point3(6.67f, 10.0f, -20.0f));
    plane_control_points.add(point3(20.0f, 10.0f, -20.0f));
    
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
// todo: close-up of a cactus, in front of the sun. Camera moving around the cactus. Duration: 3 seconds.
}

inline SceneData rolling_tumbleweed_scene()
{
// todo: canyon scene with cacti and rocks, a tumbleweed rolls by. Duration: 5 seconds.
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



