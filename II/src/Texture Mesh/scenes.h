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
    shared_ptr<instance> inst;
    vec3 translationVector;
    vec3 rotationVector;
    vec3 blur_trans = vec3(0.0f, 0.0f, 0.0f);
    float blur_rot_y = 0.0f;
    vec3 blur_rot = vec3(0.0f, 0.0f, 0.0f);
    bool has_blur_override = false;
    bool applied = false;

    void apply()
    {
        if (!inst || applied) return;
        inst->reset_to_base();
        if (rotationVector.x() != 0.0f) inst->rotate_x(rotationVector.x());
        if (rotationVector.y() != 0.0f) inst->rotate_y(rotationVector.y());
        if (rotationVector.z() != 0.0f) inst->rotate_z(rotationVector.z());
        inst->translate(translationVector);
        if (auto minst = std::dynamic_pointer_cast<moving_instance>(inst))
        {
            minst->frame_trans = translationVector;
            minst->frame_rot = rotationVector;
            minst->frame_rot_y = 0.0f;
            if (has_blur_override)
            {
                minst->trans1 = blur_trans;
                minst->rot_blur1 = blur_rot;
                if (blur_rot.y() == 0.0f && blur_rot_y != 0.0f)
                {
                    minst->rot_blur1[1] = blur_rot_y;
                }
                minst->rot_blur0 = vec3(0.0f, 0.0f, 0.0f);
                minst->rot_y0 = 0.0f;
                minst->rot_y1 = 0.0f;
            }
        }
        applied = true;
    }

    void apply_translation()
    {
        apply();
    }

    void apply_rotation()
    {
        apply();
    }

    void setTranslation(float dx, float dy, float dz)
    {
        translationVector = vec3(dx, dy, dz);
        applied = false;
    }

    void setRotation_x(float dx)
    {
        rotationVector = vec3(dx, rotationVector.y(), rotationVector.z());
        applied = false;
    }

    void setRotation_y(float dy)
    {
        rotationVector = vec3(rotationVector.x(), dy, rotationVector.z());
        applied = false;
    }

    void setRotation_z(float dz)
    {
        rotationVector = vec3(rotationVector.x(), rotationVector.y(), dz);
        applied = false;
    }
};

struct SceneData
{
    hittable_list world;
    light_list lights;
    camera cam;
    point3_list camera_positions;
    deque<InstanceTransformations> transforms;
    deque<std::vector<InstanceTransformations>> frame_transforms;
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
        point3 last_cam_pos = camera_positions.points.empty() ? cam.lookfrom : camera_positions.points.back();
        int total_frames = ceil(framerate * seconds);
        for (int i = 0; i < total_frames; i++)
        {
            camera_positions.add(last_cam_pos);
        }
    }

    void freeze(float seconds)
    {
        if (transforms.empty()) return;
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
