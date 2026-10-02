#ifndef MATRIX_H
#define MATRIX_H

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include "vec.h"
#include "point3_list.h"

using std::sqrt;

template <std::size_t R, std::size_t C>
class matrix
{
public:
  vec<C> rows[R];

  vec<C> operator[](int i) const { return rows[i]; }
  vec<C> &operator[](int i) { return rows[i]; }

  matrix operator-() const
  {
    matrix r = *this;
    return r *= -1.0f;
  }

  matrix &operator+=(const matrix<R, C> &A)
  {
    unrolled_add(A, std::make_index_sequence<R>{});
    return *this;
  }

  matrix &operator*=(float t)
  {
    unrolled_mult(t, std::make_index_sequence<R>{});
    return *this;
  }

  vec<R> operator*(const vec<C> &v) const
  {
    return unrolled_mult(v, std::make_index_sequence<R>{});
  }

  matrix &operator/=(float t)
  {
    return *this *= 1 / t;
  }

  matrix hadamard(const matrix<R, C> &A) const
  {
    return unrolled_hadamard(A, std::make_index_sequence<R>{});
  }

  // Vector times matrix
  friend vec<C> operator*(const vec<R> &v, const matrix &A)
  {
    return A.unrolled_vec_product(v, std::make_index_sequence<R>{});
  }

  // Matrix times matrix
  template <std::size_t K>
  friend matrix<R, K> operator*(const matrix &A, const matrix<C, K> &B)
  {
    return A.unrolled_product(B, std::make_index_sequence<R>{});
  }

private:
  //
  // Unrolled operations using fold expressions.
  //

  template <std::size_t... Is>
  void unrolled_add(const matrix<R, C> &A, std::index_sequence<Is...>)
  {
    ((rows[Is] += A.rows[Is]), ...);
  }

  template <std::size_t... Is>
  vec<C> unrolled_vec_product(const vec<R> &v, std::index_sequence<Is...>) const
  {
    return ((v.e[Is] * rows[Is]) + ...);
  }

  template <std::size_t... Is>
  void unrolled_mult(const float t, std::index_sequence<Is...>)
  {
    ((rows[Is] *= t), ...);
  }

  template <std::size_t... Is>
  vec<R> unrolled_mult(const vec<C> &v, std::index_sequence<Is...>) const
  {
    return vec<R>{rows[Is].dot(v)...};
  }

  template <std::size_t K, std::size_t... Is>
  matrix<R, K> unrolled_product(const matrix<C, K> &B, std::index_sequence<Is...>) const
  {
    return matrix<R, K>{rows[Is] * B...};
  }

  template <std::size_t... Is>
  matrix unrolled_hadamard(const matrix<R, C> &A, std::index_sequence<Is...>) const
  {
    return matrix{rows[Is] * A.rows[Is]...};
  }
};

// Matrix Utility Functions

template <std::size_t R, std::size_t C>
inline std::ostream &operator<<(std::ostream &out, const matrix<R, C> &M)
{
  for (std::size_t i = 0; i < R; i++)
    out << M.rows[i] << " ";
  return out;
}

template <std::size_t R, std::size_t C>
inline matrix<R, C> operator+(matrix<R, C> A, const matrix<R, C> &B)
{
  return A += B;
}

template <std::size_t R, std::size_t C>
inline matrix<R, C> operator-(matrix<R, C> A, const matrix<R, C> &B)
{
  return A += -B;
}

// Scalar times matrix
template <std::size_t R, std::size_t C>
inline matrix<R, C> operator*(float t, matrix<R, C> A)
{
  return A *= t;
}

// Matrix times scalar (commutative property)
template <std::size_t R, std::size_t C>
inline matrix<R, C> operator*(const matrix<R, C> &A, float t)
{
  return t * A;
}

// Matrix times 1/scalar and vice versa
template <std::size_t R, std::size_t C>
inline matrix<R, C> operator/(matrix<R, C> A, float t)
{
  return (1 / t) * A;
}

// Hadamard product between matrices (A.*B)
template <std::size_t R, std::size_t C>
inline matrix<R, C> hadamard(const matrix<R, C> &A, const matrix<R, C> &B)
{
  return A.hadamard(B);
}

template <std::size_t R>
inline matrix<R, 3> fromPoint3List(const point3_list& l) {
    matrix<R, 3> m{};
    for (std::size_t i = 0; i < R; i++) {
        m.rows[i] = fromVec3(l.points.at(i));
    }
    return m;
}

template <std::size_t R>
inline point3_list toPoint3List(const matrix<R, 3>& m) {
    point3_list l{};
    for (std::size_t i = 0; i < R; i++) {
        l.add(toVec3(m.rows[i]));
    }
    return l;
}

#endif
