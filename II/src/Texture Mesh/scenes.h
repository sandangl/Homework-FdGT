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
#include "point3_list.h"
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
    point3_list camera_positions;

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
