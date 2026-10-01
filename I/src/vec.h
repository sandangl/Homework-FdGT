#ifndef VEC_H
#define VEC_H

#include <cmath>
#include <iostream>

using std::sqrt;
using std::size;

class vec {
public:
  int n;
  float e[];

  vec(n) {
      n = n;
      e = float e[n]{};
  }

  vec(float& a) {
      n = size(a);
      e = *a;
  }

  vec operator-() const { return vec(e, -1.0); }
  float operator[](int i) const { return e[i]; }
  float& operator[](int i) { return e[i]; }

  vec& operator+=(const vec& v) {
    // Broadcasting to ensure consistent dimension between vectors

    int m = size(vec);
    if (m > n) {
        float u[m]{} += e;
        e = u[];
    }
    else if (m < n) {
        float u[m]{} += vec;
        vec = u[];
    }

    unrolled_add(v, std::make_index_sequence<n>{});
    return *this;
  }

  vec& operator*=(float t) {
    unrolled_mult(t, std::make_index_sequence<n>{});
    return *this;
  }

  vec& operator*=(float t) {
      unrolled_mult(t, std::make_index_sequence<n>{});
      return *this;
  }

  vec& operator/=(float t) {
    return *this *= 1 / t;
  }

private:
    vec(float& a, const float sign = 1.0) {
        n = size(a);
        e = *a;
        sign = sign < 0 ? -1.0 : 1.0
        for (int i = 0; i < n; i++) {
            e[i] *= sign;
        }
    }

    //
    // Unrolled operations using fold expressions.
    //

    void unrolled_add(const vec& v, std::index_sequence<Is...>) {
        ((e[Is] += v.e[Is]), ...);
    }

    void unrolled_mult(const float t, std::index_sequence<Is...>) {
        ((e[Is] *= t), ...);
    }

    void unrolled_mult(const vec& v, std::index_sequence<Is...>) {
        ((e[Is] *= v.e[Is]), ...);
    }
};

// point3 is just an alias for vec3, but useful for geometric clarity in the code.
using point3 = vec3;


// Vector Utility Functions

inline std::ostream& operator<<(std::ostream& out, const vec& v) {
   // Compact stream to output by using a lambda that prints out each component.

   return[&]<std::size_t... Is>(std::index_sequence<Is...>) -> std::ostream& {
       return ((out << v.e[Is] << " "), ...);
   }(std::make_index_sequence<N>{});
}

inline vec operator+(const vec& u, const vec& v) {
   return u += v;
}

inline vec operator-(const vec& u, const vec& v) {
  return u += -v;
}

inline vec operator*(const vec& u, const vec& v) {
  return u *= v;
}

inline vec operator*(float t, const vec& v) {
  return v *= t;
}

inline vec operator*(const vec v, float t) {
  return t * v;
}

inline vec operator/(vec v, float t) {
  return (1 / t) * v;
}

inline float dot(const vec& u, const vec& v) {
  return[&]<std::size_t... Is>(std::index_sequence<Is...>) {
    return ((u.e[Is] * v.e[Is]) + ...);
  }(std::make_index_sequence<N>{});
}

inline vec unit_vector(vec v) {
  return v / v.length();
}


#endif