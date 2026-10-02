#ifndef VEC_H
#define VEC_H

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

using std::sqrt;

template <std::size_t N>
class vec {
public:
  float e[N]{};

  float operator[](int i) const { return e[i]; }
  float& operator[](int i) { return e[i]; }

  vec operator-() const {
    vec r = *this;
    return r *= -1.0f;
  }

  template <std::size_t M>
  vec& operator+=(const vec<M>& v) {
    unrolled_add(v, std::make_index_sequence<std::min(N, M)>{});
    return *this;
  }

  vec& operator*=(float t) {
    unrolled_mult(t, std::make_index_sequence<N>{});
    return *this;
  }

  template <std::size_t M>
  vec& operator*=(const vec<M>& v) {
    unrolled_mult(v, std::make_index_sequence<std::min(N, M)>{});
    return *this;
  }

  vec& operator/=(float t) {
    return *this *= 1 / t;
  }

  template <std::size_t M>
  float dot(const vec<M>& v) const {
    return unrolled_dot(v, std::make_index_sequence<std::min(N, M)>{});
  }

private:

    //
    // Unrolled operations using fold expressions.
    //

    template <std::size_t M, std::size_t... Is>
    void unrolled_add(const vec<M>& v, std::index_sequence<Is...>) {
      ((e[Is] += v.e[Is]), ...);
    }

    template <std::size_t... Is>
    void unrolled_mult(const float t, std::index_sequence<Is...>) {
      ((e[Is] *= t), ...);
    }

    template <std::size_t M, std::size_t... Is>
    void unrolled_mult(const vec<M>& v, std::index_sequence<Is...>) {
      ((e[Is] *= v.e[Is]), ...);
    }

    template <std::size_t M, std::size_t... Is>
    float unrolled_dot(const vec<M>& v, std::index_sequence<Is...>) const {
      return ((e[Is] * v.e[Is]) + ...);
    }

};

// Vector Utility Functions

template <std::size_t N>
inline std::ostream& operator<<(std::ostream& out, const vec<N>& v) {
   for (std::size_t i = 0; i < N; i++)
     out << v.e[i] << " ";
   return out;
}

template <std::size_t N>
inline vec<N> operator+(vec<N> u, const vec<N>& v) {
   return u += v;
}

template <std::size_t N>
inline vec<N> operator-(vec<N> u, const vec<N>& v) {
  return u += -v;
}

template <std::size_t N>
inline vec<N> operator*(vec<N> u, const vec<N>& v) {
  return u *= v;
}

template <std::size_t N>
inline vec<N> operator*(float t, vec<N> v) {
  return v *= t;
}

template <std::size_t N>
inline vec<N> operator*(const vec<N>& v, float t) {
  return t * v;
}

template <std::size_t N>
inline vec<N> operator/(vec<N> v, float t) {
  return (1 / t) * v;
}

template <std::size_t N>
inline float dot(const vec<N>& u, const vec<N>& v) {
  return u.dot(v);
}

#endif
