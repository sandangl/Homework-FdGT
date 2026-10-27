#pragma once
// Mesh triangolare con coordinate di texture, versione SENZA BVH: l'intersezione raggio-mesh
// prova tutti i triangoli (dopo il test sul bounding box). E' l'alternativa didattica a mesh.h:
// si seleziona con USE_TINYBVH 0 in cima a main.cpp.
#include <vector>
#include <cstdio>
#include <iostream>
#include "point2.h"
#include "vec3.h"
#include "aabb.h"
#include "ray.h"
#include "hittable.h"
#include "interval.h"
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

	mesh(const char* filename, const char* basepath = NULL, bool triangulate = true) {
		loaded = load_mesh(filename, basepath, triangulate);
	};

	bool load_mesh(const char* filename, const char* basepath, bool triangulate);

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const;
	virtual bool hit_shadow(const ray& r, interval ray_t) const;

	virtual bool bounding_box(aabb& box) const;
};

bool mesh::bounding_box(aabb& box) const {
	box = aabb_mesh;
	return true;
}

// NB: gli argomenti di default vanno solo nella dichiarazione dentro la classe,
// ripeterli qui e' un errore di compilazione (C2572 / "default argument given for parameter").
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
	// NB: FLT_MIN e' il piu' piccolo float POSITIVO, non il valore piu' negativo, e inizializzare
	// max a FLT_MAX rende il bounding box infinito (il test sul box non scarta mai nulla).
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

	return true;
}


// Moller-Trumbore: restituisce anche i pesi baricentrici u (su v1) e v (su v2); il peso su v0 vale 1 - u - v.
bool triangle_intersection(const ray& r, float tmin, float tmax, hit_record& rec, const point3& v0, const point3& v1, const point3& v2, float& u, float& v)
{
	const float EPSILON = 0.0000001f;
	vec3 edge1, edge2, h, s, q;
	float a, f;
	edge1 = v1 - v0;
	edge2 = v2 - v0;
	h = cross(r.d, edge2);
	a = dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;
	f = 1 / a;
	s = r.o - v0;
	u = f * (dot(s, h));
	if (u < 0.0f || u > 1.0f)
		return false;
	q = cross(s, edge1);
	v = f * dot(r.d, q);
	if (v < 0.0f || u + v > 1.0f)
		return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * dot(edge2, q);
	if (t > tmin && t < tmax) {
		if (t > EPSILON) // ray intersection
		{
			rec.normal = unit_vector(cross(v1 - v0, v2 - v0));
			rec.t = t;
			rec.p = r.at(rec.t);
			return true;
		}
	}
	// This means that there is a line intersection but not a ray intersection.
	return false;
}


bool mesh::hit(const ray& ray, interval ray_t, hit_record& rec) const
{
	bool hit_anything = false;
	hit_record temp_rec;
	float closest_so_far = ray_t.max;
	float u, v;

	// Check intersection with bound boux
	if (aabb_mesh.hit(ray, ray_t) == false)
		return false;

	for (int i = 0; i < num_faces; i++)
	{
		int i0 = vertex_faces[0][3 * i + 0];
		int i1 = vertex_faces[0][3 * i + 1];
		int i2 = vertex_faces[0][3 * i + 2];

		point3 v0 = vertices[i0];
		point3 v1 = vertices[i1];
		point3 v2 = vertices[i2];

		if (triangle_intersection(ray, ray_t.min, closest_so_far, temp_rec, v0, v1, v2, u, v)) {
			hit_anything = true;
			closest_so_far = temp_rec.t;
			rec = temp_rec;

			vec3 bary(1.0f - u - v, u, v);

			// normale interpolata, se il file ha le normali (indice -1 = vertice senza normale)
			if (areNormals)
			{
				i0 = vertex_faces[1][3 * i + 0];
				i1 = vertex_faces[1][3 * i + 1];
				i2 = vertex_faces[1][3 * i + 2];

				if (i0 >= 0 && i1 >= 0 && i2 >= 0)
					rec.normal = unit_vector(normals[i0] * bary[0] + normals[i1] * bary[1] + normals[i2] * bary[2]);
			}

			// coordinate di texture interpolate; senza vt restano (0, 0)
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
		}
	}
	return hit_anything;
}

bool mesh::hit_shadow(const ray& ray, interval ray_t) const
{
	hit_record temp_rec;
	float u, v;

	if (aabb_mesh.hit(ray, ray_t) == false)
		return false;

	for (int i = 0; i < num_faces; i++)
	{
		int i0 = vertex_faces[0][3 * i + 0];
		int i1 = vertex_faces[0][3 * i + 1];
		int i2 = vertex_faces[0][3 * i + 2];

		point3 v0 = vertices[i0];
		point3 v1 = vertices[i1];
		point3 v2 = vertices[i2];

		if (triangle_intersection(ray, ray_t.min, ray_t.max, temp_rec, v0, v1, v2, u, v))
			return true;
	}
	return false;
}
