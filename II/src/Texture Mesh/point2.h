#pragma once

struct point2
{
	union {
		float		x, u;
	};
	union {
		float		y, v;
	};

	point2() = default;

	point2(float a, float b) { x = a; y = b; }

	float& operator [](int i)
	{
		return ((&x)[i]);
	}

	const float& operator [](int i) const
	{
		return ((&x)[i]);
	}
};

inline point2 operator *(const point2& v, float s)
{
	return (point2(v.x * s, v.y * s));
}

inline point2 operator *(float s, const point2& v)
{
	return (point2(v.x * s, v.y * s));
}

inline point2 operator +(const point2& a, const point2& b)
{
	return (point2(a.x + b.x, a.y + b.y));
}