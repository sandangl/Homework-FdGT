#pragma once
// Mesh triangolare con coordinate di texture (lezione 10).
// L'intersezione raggio-mesh usa TinyBVH (https://github.com/jbikker/tinybvh, licenza MIT, header in tinybvh/):
// il BVH e' costruito una volta in load_mesh, poi Intersect / IsOccluded sostituiscono il ciclo su tutti
// i triangoli. La versione senza BVH e' in mesh_bruteforce.h (USE_TINYBVH 0 in cima a main.cpp).
// NB: TinyBVH richiede C++17 (costruzione in parallelo); con C++14 definire NO_THREADED_BUILDS qui sotto.
#include <vector>
#include <cstdio>
#include <iostream>
#include "point2.h"
#include "vec3.h"
#include "aabb.h"
#include "ray.h"
#include "hittable.h"
#include "interval.h"
// TinyBVH 1.9.1 include "windows.h" DENTRO il proprio namespace (tiny_bvh_base.h, pool di thread):
// se windows.h non e' gia' stato incluso, tutte le API Windows finiscono in tinybvh:: e il
// successivo #include <windows.h> di camera.h viene saltato (ppl.h non trova HRESULT).
// Includendolo qui, a livello globale, l'inclusione interna a TinyBVH diventa un no-op.
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX            // niente macro min/max (si usano std::min / std::max)
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // esclude rpcndr.h, il cui "byte" e' ambiguo con std::byte (C++17) dopo "using namespace std"
#endif
#include <windows.h>
#endif
#define TINYBVH_IMPLEMENTATION
#include "tinybvh/tiny_bvh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace std;

class mesh : public hittable {
public:

	vector<point3> 				vertices;				// posizioni dei vertici (righe v)
	vector<vec3> 					normals;				// normali lette dal file (righe vn), se ci sono
	vector<point2>				textures;				// coordinate di texture (righe vt), se ci sono
	vector<vector<int> > 	vertex_faces;		// per ogni vertice di ogni faccia: [0] indice della posizione, [1] della normale, [2] delle (u, v)
	int num_vertices = 0;
	int num_normals = 0;
	int num_textures = 0;
	int num_shapes = 0;
	int num_faces = 0;
	aabb aabb_mesh;

	bool areNormals = false;
	bool areTextures = false;
	bool loaded = false;									// true se il file .obj e' stato caricato

	vector<tinybvh::bvhvec4> tri_vertices;	// tre vertici per triangolo: TinyBVH li legge da qui (non li copia)
	tinybvh::BVH tiny;

	mesh(const char* filename, const char* basepath = NULL, bool triangulate = true) {
		loaded = load_mesh(filename, basepath, triangulate);
	};

	bool load_mesh(const char* filename, const char* basepath, bool triangulate);

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const;
	virtual bool hit_shadow(const ray& r, interval ray_t) const;

	virtual bool bounding_box(aabb& box) const;

	// Il raggio di TinyBVH parte da t = 0: per rispettare ray_t.min si sposta l'origine
	// di ray_t.min * d e si accorcia il raggio della stessa quantita'.
	static tinybvh::Ray to_tinybvh(const ray& r, interval ray_t) {
		point3 o = r.origin() + ray_t.min * r.direction();
		vec3 d = r.direction();
		return tinybvh::Ray(tinybvh::bvhvec3(o[0], o[1], o[2]),
		                    tinybvh::bvhvec3(d[0], d[1], d[2]), ray_t.max - ray_t.min);
	}
};

bool mesh::bounding_box(aabb& box) const {
	box = aabb_mesh;
	return true;
}

bool mesh::load_mesh(const char* filename, const char* basepath, bool triangulate)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	std::string war;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &war, &err, filename, basepath, triangulate);

	if (!war.empty()) {
		std::cerr << "OBJ warning: " << war << std::endl;
	}

	if (!err.empty()) {
		std::cerr << "OBJ error: " << err << std::endl;
		return false;
	}

	if (!ret) {
		printf("Failed to load/parse .obj.\n");
		return false;
	}

	num_vertices = (int)(attrib.vertices.size() / 3);
	num_normals = (int)(attrib.normals.size() / 3);
	num_textures = (int)(attrib.texcoords.size() / 2);
	num_shapes = (int)shapes.size();

	// Intervallo "vuoto" (+inf, -inf): il primo vertice lo inizializza correttamente.
	// NB: FLT_MIN e' il piu' piccolo float POSITIVO, non il valore piu' negativo.
	aabb_mesh.x = interval();
	aabb_mesh.y = interval();
	aabb_mesh.z = interval();

	for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
		point3 p = point3(attrib.vertices[3 * v + 0], attrib.vertices[3 * v + 1], attrib.vertices[3 * v + 2]);

		aabb_mesh.x.min = ffmin(p[0], aabb_mesh.x.min);
		aabb_mesh.y.min = ffmin(p[1], aabb_mesh.y.min);
		aabb_mesh.z.min = ffmin(p[2], aabb_mesh.z.min);
		aabb_mesh.x.max = ffmax(p[0], aabb_mesh.x.max);
		aabb_mesh.y.max = ffmax(p[1], aabb_mesh.y.max);
		aabb_mesh.z.max = ffmax(p[2], aabb_mesh.z.max);

		vertices.push_back(p);
	}

	if (attrib.normals.size() > 0)
	{
		areNormals = true;
		for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
			normals.push_back(vec3(attrib.normals[3 * v + 0], attrib.normals[3 * v + 1], attrib.normals[3 * v + 2]));
		}
	}

	if (attrib.texcoords.size() > 0)
	{
		areTextures = true;
		for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
			textures.push_back(point2(attrib.texcoords[2 * v + 0], attrib.texcoords[2 * v + 1]));
		}
	}

	vertex_faces.push_back(vector<int>());
	vertex_faces.push_back(vector<int>());
	vertex_faces.push_back(vector<int>());

	num_faces = 0;
	// For each shape
	for (size_t s = 0; s < shapes.size(); s++) {
		int index_offset = 0;

		// For each face
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			num_faces++;

			// For each vertex in the face
			for (int v = 0; v < fv; v++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				vertex_faces[0].push_back(idx.vertex_index);
				vertex_faces[1].push_back(idx.normal_index);
				vertex_faces[2].push_back(idx.texcoord_index);
			}

			index_offset += fv;
		}
	}

	// I triangoli nel formato di TinyBVH (tre bvhvec4 per triangolo) e la costruzione del BVH
	for (int k = 0; k < 3 * num_faces; k++) {
		point3 p = vertices[vertex_faces[0][k]];
		tri_vertices.push_back(tinybvh::bvhvec4(p[0], p[1], p[2], 0.0f));
	}
	if (num_faces > 0)
		tiny.Build(tri_vertices.data(), num_faces);

	return true;
}

bool mesh::hit(const ray& r, interval ray_t, hit_record& rec) const
{
	if (num_faces == 0) return false;

	tinybvh::Ray tr = to_tinybvh(r, ray_t);
	tiny.Intersect(tr);
	if (tr.hit.t >= ray_t.max - ray_t.min)
		return false;                               // nessun triangolo nell'intervallo

	int i = tr.hit.prim;                            // indice del triangolo colpito
	float u = tr.hit.u, v = tr.hit.v;               // pesi baricentrici beta e gamma (Moller-Trumbore)
	vec3 bary(1.0f - u - v, u, v);

	rec.t = ray_t.min + tr.hit.t;                   // si torna al parametro del nostro raggio
	rec.p = r.at(rec.t);

	// normale geometrica (come triangle_intersection della lezione 8)
	int i0 = vertex_faces[0][3 * i + 0];
	int i1 = vertex_faces[0][3 * i + 1];
	int i2 = vertex_faces[0][3 * i + 2];
	rec.normal = unit_vector(cross(vertices[i1] - vertices[i0], vertices[i2] - vertices[i0]));

	// normale interpolata, se il file ha le normali
	if (areNormals)
	{
		i0 = vertex_faces[1][3 * i + 0];
		i1 = vertex_faces[1][3 * i + 1];
		i2 = vertex_faces[1][3 * i + 2];
		if (i0 >= 0 && i1 >= 0 && i2 >= 0)
			rec.normal = unit_vector(normals[i0] * bary[0] + normals[i1] * bary[1] + normals[i2] * bary[2]);
	}

	// coordinate di texture interpolate (lezione 10); senza vt restano (0, 0)
	rec.u = 0.0f;
	rec.v = 0.0f;
	if (areTextures)
	{
		i0 = vertex_faces[2][3 * i + 0];
		i1 = vertex_faces[2][3 * i + 1];
		i2 = vertex_faces[2][3 * i + 2];
		if (i0 >= 0 && i1 >= 0 && i2 >= 0) {
			point2 uv = textures[i0] * bary[0] + textures[i1] * bary[1] + textures[i2] * bary[2];
			rec.u = uv.x;
			rec.v = uv.y;
		}
	}
	return true;
}

bool mesh::hit_shadow(const ray& r, interval ray_t) const
{
	if (num_faces == 0) return false;
	return tiny.IsOccluded(to_tinybvh(r, ray_t));
}
