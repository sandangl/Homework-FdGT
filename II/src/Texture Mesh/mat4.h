#pragma once
#include "vec3.h"

class mat4 {

public:

	float	m[4][4];										// elements

	mat4(void) {
		m[0][0] = 1.0; m[0][1] = 0.0; m[0][2] = 0.0; m[0][3] = 0.0;
		m[1][0] = 0.0; m[1][1] = 1.0; m[1][2] = 0.0; m[1][3] = 0.0;
		m[2][0] = 0.0; m[2][1] = 0.0; m[2][2] = 1.0; m[2][3] = 0.0;
		m[3][0] = 0.0; m[3][1] = 0.0; m[3][2] = 0.0; m[3][3] = 1.0;
	}

	mat4(const mat4& mat);						// copy constructor

	~mat4(void) {}

	float& operator ()(int i, int j) {
		return (m[i][j]);
	}

	const float& operator ()(int i, int j) const {
		return (m[i][j]);
	}

	mat4& operator= (const mat4& rhs);

	mat4 operator* (const mat4& mat) const;

	mat4 operator/ (const float d);
};

mat4::mat4(const mat4& mat) {
	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 4; y++)
			m[x][y] = mat.m[x][y];
}

mat4& mat4::operator= (const mat4& rhs) {
	if (this == &rhs)
		return (*this);

	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 4; y++)
			m[x][y] = rhs.m[x][y];

	return (*this);
}

mat4 mat4::operator* (const mat4& mat) const {
	mat4 	product;

	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++) {
			float sum = 0.0;

			for (int j = 0; j < 4; j++)
				sum += m[x][j] * mat.m[j][y];

			product.m[x][y] = sum;
		}

	return (product);
}

mat4 mat4::operator/ (const float d) {
	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 4; y++)
			m[x][y] = m[x][y] / d;

	return (*this);
}

void set_identity(mat4& mat) {
	mat.m[0][0] = 1.0; mat.m[0][1] = 0.0; mat.m[0][2] = 0.0; mat.m[0][3] = 0.0;
	mat.m[1][0] = 0.0; mat.m[1][1] = 1.0; mat.m[1][2] = 0.0; mat.m[1][3] = 0.0;
	mat.m[2][0] = 0.0; mat.m[2][1] = 0.0; mat.m[2][2] = 1.0; mat.m[2][3] = 0.0;
	mat.m[3][0] = 0.0; mat.m[3][1] = 0.0; mat.m[3][2] = 0.0; mat.m[3][3] = 1.0;
}

mat4 transponse(const mat4& mat) {
	mat4 transp;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			transp.m[i][j] = mat.m[j][i];
			transp.m[j][i] = mat.m[i][j];
		}
	}
	return transp;
}

vec3 operator* (const mat4& mat, const vec3& v) {
	return (vec3(mat.m[0][0] * v[0] + mat.m[0][1] * v[1] + mat.m[0][2] * v[2],
		mat.m[1][0] * v[0] + mat.m[1][1] * v[1] + mat.m[1][2] * v[2],
		mat.m[2][0] * v[0] + mat.m[2][1] * v[1] + mat.m[2][2] * v[2]));
}


point3 multiply(const mat4& mat, const point3& p) {
	//point3 operator* (const mat4& mat, const point3& p) {
	return (point3(mat.m[0][0] * p[0] + mat.m[0][1] * p[1] + mat.m[0][2] * p[2] + mat.m[0][3],
		mat.m[1][0] * p[0] + mat.m[1][1] * p[1] + mat.m[1][2] * p[2] + mat.m[1][3],
		mat.m[2][0] * p[0] + mat.m[2][1] * p[1] + mat.m[2][2] * p[2] + mat.m[2][3]));
}