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

		if (hittable_ptr->hit_shadow(inv_ray, ray_t))
			return true;

		return false;
	}


	void identity() {
		set_identity(current_matrix);
		set_identity(inverse_matrix);
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

private:
	hittable* hittable_ptr;			// object to be transformed
	mat4			inverse_matrix;		// inverse transformation matrix
	mat4			current_matrix; 	// current transformation matrix
	material* mat;							// material					
};

#endif