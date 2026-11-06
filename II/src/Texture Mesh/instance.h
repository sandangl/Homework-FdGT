#ifndef INSTANCE_H
#define INSTANCE_H

#include "ray.h"
#include "material.h"
#include "mat4.h"
#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class instance : public hittable {
public:

	instance(hittable* object, material* m) {
		hittable_ptr = object;
		mat = m;
		save_base_transform();
	}

	void setMaterial(material* m) {
		mat = m;
	}

	material* getMaterial() {
		return(mat);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		ray inv_ray;

		inv_ray.o = multiply(inverse_matrix, r.o);
		inv_ray.d = inverse_matrix * r.d;
		inv_ray.tau = r.time();

		if (hittable_ptr->hit(inv_ray, ray_t, rec)) {
			rec.p = multiply(current_matrix, rec.p);
			rec.normal = unit_vector(transponse(inverse_matrix) * rec.normal);
			rec.m = mat;

			return true;
		}

		return false;
	}


	bool hit_shadow(const ray& r, interval ray_t) const {
		ray inv_ray;

		inv_ray.o = multiply(inverse_matrix, r.o);
		inv_ray.d = inverse_matrix * r.d;
		inv_ray.tau = r.time();

		if (hittable_ptr->hit_shadow(inv_ray, ray_t))
			return true;

		return false;
	}


	void identity() {
		set_identity(current_matrix);
		set_identity(inverse_matrix);
	}

	void save_base_transform() {
		base_current_matrix = current_matrix;
		base_inverse_matrix = inverse_matrix;
	}

	void reset_to_base() {
		current_matrix = base_current_matrix;
		inverse_matrix = base_inverse_matrix;
	}

	void translate(const vec3& trans) {
		translate(trans[0], trans[1], trans[2]);
	}

	void translate(const float dx, const float dy, const float dz) {

		mat4 inv_translation_matrix;				// temporary inverse translation matrix	

		inv_translation_matrix.m[0][3] = -dx;
		inv_translation_matrix.m[1][3] = -dy;
		inv_translation_matrix.m[2][3] = -dz;

		inverse_matrix = inverse_matrix * inv_translation_matrix;

		mat4 translation_matrix;					// temporary translation matrix4D	

		translation_matrix.m[0][3] = dx;
		translation_matrix.m[1][3] = dy;
		translation_matrix.m[2][3] = dz;

		current_matrix = translation_matrix * current_matrix;
	}

	void scale(const vec3& s) {
		scale(s[0], s[1], s[2]);
	}

	void scale(const float a, const float b, const float c) {

		mat4 inv_scaling_matrix;					// temporary inverse scaling matrix4D

		inv_scaling_matrix.m[0][0] = 1.0f / a;
		inv_scaling_matrix.m[1][1] = 1.0f / b;
		inv_scaling_matrix.m[2][2] = 1.0f / c;

		inverse_matrix = inverse_matrix * inv_scaling_matrix;

		mat4 scaling_matrix;						// temporary scaling matrix4D

		scaling_matrix.m[0][0] = a;
		scaling_matrix.m[1][1] = b;
		scaling_matrix.m[2][2] = c;

		current_matrix = scaling_matrix * current_matrix;
	}

	void rotate_x(const float theta) {

		float sin_theta = sin(degrees_to_radians(theta));
		float cos_theta = cos(degrees_to_radians(theta));

		mat4 inv_x_rotation_matrix;					// temporary inverse rotation matrix4D about x axis

		inv_x_rotation_matrix.m[1][1] = cos_theta;
		inv_x_rotation_matrix.m[1][2] = sin_theta;
		inv_x_rotation_matrix.m[2][1] = -sin_theta;
		inv_x_rotation_matrix.m[2][2] = cos_theta;

		inverse_matrix = inverse_matrix * inv_x_rotation_matrix;

		mat4 x_rotation_matrix;						// temporary rotation matrix4D about x axis

		x_rotation_matrix.m[1][1] = cos_theta;
		x_rotation_matrix.m[1][2] = -sin_theta;
		x_rotation_matrix.m[2][1] = sin_theta;
		x_rotation_matrix.m[2][2] = cos_theta;

		current_matrix = x_rotation_matrix * current_matrix;
		//current_matrix = current_matrix * x_rotation_matrix;
	}

	void rotate_y(const float theta) {

		float sin_theta = sin(degrees_to_radians(theta));
		float cos_theta = cos(degrees_to_radians(theta));

		mat4 inv_y_rotation_matrix;					// temporary inverse rotation matrix4D about y axis

		inv_y_rotation_matrix.m[0][0] = cos_theta;
		inv_y_rotation_matrix.m[0][2] = -sin_theta;
		inv_y_rotation_matrix.m[2][0] = sin_theta;
		inv_y_rotation_matrix.m[2][2] = cos_theta;

		inverse_matrix = inverse_matrix * inv_y_rotation_matrix;

		mat4 y_rotation_matrix;						// temporary rotation matrix4D about y axis

		y_rotation_matrix.m[0][0] = cos_theta;
		y_rotation_matrix.m[0][2] = sin_theta;
		y_rotation_matrix.m[2][0] = -sin_theta;
		y_rotation_matrix.m[2][2] = cos_theta;

		current_matrix = y_rotation_matrix * current_matrix;
	}

	void rotate_z(const float theta) {
		float sin_theta = sin(degrees_to_radians(theta));
		float cos_theta = cos(degrees_to_radians(theta));

		mat4 inv_z_rotation_matrix;					// temporary inverse rotation matrix4D about y axis	

		inv_z_rotation_matrix.m[0][0] = cos_theta;
		inv_z_rotation_matrix.m[0][1] = sin_theta;
		inv_z_rotation_matrix.m[1][0] = -sin_theta;
		inv_z_rotation_matrix.m[1][1] = cos_theta;

		inverse_matrix = inverse_matrix * inv_z_rotation_matrix;

		mat4 z_rotation_matrix;						// temporary rotation matrix4D about y axis

		z_rotation_matrix.m[0][0] = cos_theta;
		z_rotation_matrix.m[0][1] = -sin_theta;
		z_rotation_matrix.m[1][0] = sin_theta;
		z_rotation_matrix.m[1][1] = cos_theta;

		current_matrix = z_rotation_matrix * current_matrix;
	}
	
protected:
	hittable* hittable_ptr;			// object to be transformed
	mat4			inverse_matrix;		// inverse transformation matrix
	mat4			current_matrix; 	// current transformation matrix
	mat4			base_inverse_matrix;	// base inverse matrix before animation
	mat4			base_current_matrix;	// base matrix before animation
	material* mat;							// material					
};

class moving_instance : public instance
{
public:
	float t0 = 0.0f, t1 = 0.5f;
	vec3 trans0 = vec3(0.0f, 0.0f, 0.0f);
	vec3 trans1 = vec3(0.0f, 0.0f, 0.0f);
	float rot_y0 = 0.0f, rot_y1 = 0.0f;
	vec3 scale_vec = vec3(1.0f, 1.0f, 1.0f);
	vec3 base_rot = vec3(0.0f, 0.0f, 0.0f);

	vec3 frame_trans = vec3(0.0f, 0.0f, 0.0f);
	float frame_rot_y = 0.0f;

	moving_instance(hittable* obj, material* m, float _t0 = 0.0f, float _t1 = 0.5f,
	                vec3 _trans0 = vec3(0.0f, 0.0f, 0.0f), vec3 _trans1 = vec3(0.0f, 0.0f, 0.0f),
	                float _rot_y0 = 0.0f, float _rot_y1 = 0.0f,
	                vec3 _scale = vec3(1.0f, 1.0f, 1.0f), vec3 _base_rot = vec3(0.0f, 0.0f, 0.0f))
		: instance(obj, m), t0(_t0), t1(_t1),
		  trans0(_trans0), trans1(_trans1),
		  rot_y0(_rot_y0), rot_y1(_rot_y1),
		  scale_vec(_scale), base_rot(_base_rot),
		  frame_trans(vec3(0.0f, 0.0f, 0.0f)), frame_rot_y(0.0f)
	{
		scale(scale_vec);
		if (base_rot.x() != 0.0f) rotate_x(base_rot.x());
		if (base_rot.y() != 0.0f) rotate_y(base_rot.y());
		if (base_rot.z() != 0.0f) rotate_z(base_rot.z());
		save_base_transform();
	}

	instance get_instance_at(float tau) const {
		float s = (t1 > t0) ? std::clamp((tau - t0) / (t1 - t0), 0.0f, 1.0f) : 0.0f;
		vec3 cur_trans = frame_trans + (1.0f - s) * trans0 + s * trans1;
		float cur_rot_y = frame_rot_y + (1.0f - s) * rot_y0 + s * rot_y1;

		instance inst(hittable_ptr, mat);
		inst.identity();
		inst.scale(scale_vec);
		if (base_rot.x() != 0.0f) inst.rotate_x(base_rot.x());
		if (base_rot.y() + cur_rot_y != 0.0f) inst.rotate_y(base_rot.y() + cur_rot_y);
		if (base_rot.z() != 0.0f) inst.rotate_z(base_rot.z());
		inst.translate(cur_trans);
		return inst;
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		instance inst = get_instance_at(r.time());
		return inst.hit(r, ray_t, rec);
	}

	bool hit_shadow(const ray& r, interval ray_t) const override {
		instance inst = get_instance_at(r.time());
		return inst.hit_shadow(r, ray_t);
	}
};

#endif