#pragma once

#include <list>
#include <memory>
#include <iostream>
#include <functional>
#include <deque>

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
#include "point3_list.h"
#include "instance.h"

#if USE_TINYBVH
#include "mesh.h"
#else
#include "mesh_bruteforce.h"
#endif

using std::make_shared;

struct InstanceTransformations
{
    instance inst;
    vec3 translationVector;
    vec3 rotationVector;

    void apply_translation()
    {
        inst.translate(translationVector);
    }

    void setTranslation(float dx, float dy, float dz)
    {
        vec3 translationVector = vec3(dx, dy, dz);
    }

    void apply_rotation()
    {
        inst.rotate_x(rotationVector.x());
        inst.rotate_y(rotationVector.y());
        inst.rotate_z(rotationVector.z());
    }

    void setRotation_x(float dx)
    {
        rotationVector = vec3(dx, 0, 0);
    }

    void setRotation_y(float dy)
    {
        rotationVector = vec3(0, dy, 0);
    }

    void setRotation_z(float dz)
    {
        rotationVector = vec3(0, 0, dz);
    }
};

struct SceneData
{
    hittable_list world;
    light_list lights;
    camera cam;
    point3_list camera_positions;
    deque<InstanceTransformations> transforms;
    int framerate = 30;

    std::list<mesh*> meshes;
    std::list<texture*> textures;
    std::list<material*> materials;
    std::list<light*> allocated_lights;
    std::list<ext_light*> allocated_ext_lights;
    std::list<hittable*> allocated_hittables;

    void cleanup()
    {
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

    void wait(float seconds)
    {
        point3 last_cam_pos = camera_positions.points.at(camera_positions.length() - 1);
        int total_frames = ceil(framerate * seconds);
        for (int i = 0; i < total_frames; i++)
        {
            camera_positions.add(last_cam_pos);
        }
    }

    void freeze(float seconds)
    {
        InstanceTransformations last = transforms.back();
        int total_frames = ceil(framerate * seconds);
        for (int i = 0; i < total_frames; i++)
        {
            transforms.push_back(last);
        }
    }
};

inline void cleanup_scene(SceneData& scene)
{
    scene.cleanup();
}

using SceneFactory = std::function<SceneData()>;
