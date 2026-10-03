# First Homework

## Introduction

The codebase (`./src/`) is based on the 5th lecture version of the graphics engine, available [here](https://informatica.unibas.it/moodle/pluginfile.php/35971/mod_folder/content/0/Camera.zip?forcedownload=1).

This project implements camera system improvements:

- **Anaglyph stereogram** (medium difficulty)
- **Revolving camera point of view** (hard difficulty)

## 1. Anaglyph Stereogram

The red-cyan anaglyph is implemented by converting the left-eye and right-eye views to grayscale using PAL standard luminance weights (0.222 R + 0.707 G + 0.071 B).

To compose the stereogram, the left-eye luminance is assigned exclusively to the red channel, while the right-eye luminance is assigned to both the green and blue (cyan) channels. The method `toAnaglyph()` performs this channel isolation:

```C++
color toAnaglyph(color rayColor, bool isLeft) {
    // Each channel gets multiplied by a constant that mimics human perception of red, blue and green.
    float gray = 0.222f * rayColor[0] + 0.707f * rayColor[1] + 0.071f * rayColor[2];
    
    rayColor[0] = isLeft ? gray : 0.0f;
    rayColor[1] = isLeft ? 0.0f : gray;
    rayColor[2] = isLeft ? 0.0f : gray;

    return rayColor;
}
```

The rendering pipeline splits the viewpoint into two camera positions corresponding to the left and right eyes. The two viewpoints are separated by horizontal offset `e`, representing the interocular distance:

```C++
void render(const hittable& world, const bool toggle_anaglyph = false, const float e = 0.0) {
    ...
    if (toggle_anaglyph) {
        ray rr = get_ray(i, j, e);
        ray rl = get_ray(i, j, -e);
        pixel_color += toAnaglyph(ray_color(rl, world, true), true) + toAnaglyph(ray_color(rr, world, true), false);
    }
    ...
}

ray get_ray(int i, int j, float e = 0.0) const {
  // Get a randomly sampled camera ray for the pixel at location i,j.

  vec3 pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
  vec3 pixel_sample = pixel_center + pixel_sample_square();

  point3 ray_origin = camera_center + 0.5 * e * pixel_delta_u;
  vec3 ray_direction = pixel_sample - ray_origin;

  return ray(ray_origin, ray_direction);
}
```

Because `e` is signed, inverting its sign reverses the parallax effect. A positive value produces a perceived pop-out effect in front of the screen, whereas a negative value creates depth receding into the screen.

Sample renders are available in `deliverables/`:
- `anaglyph@246px.bmp`: standard human interocular distance (~6.5 cm, ~246 px at 1280x720)
- `anaglyph@1%.bmp`: subtle depth effect (1% viewport width)
- `anaglyph@5%.bmp`: pronounced depth effect (5% viewport width)
- `inverted_anaglyph@246px.bmp`: standard human interocular distance (~6.5 cm, ~246 px at 1280x720)
- `inverted_anaglyph@1%.bmp`: subtle depth effect (1% viewport width)
- `inverted_anaglyph@5%.bmp`: pronounced depth effect (5% viewport width)

## 2. Revolving camera point of view

To achieve this, we implemented a revolving camera system using a Catmull-Rom spline path. 
...
The implementation followed three different parts:

### a. Implementation of `vec` and `matrix`

In order to achieve maximum optimization during Catmull-Rom spline point interpolation, we propose a matrix-based version as follows:

Given the time vector:
$$
T = \begin{bmatrix}
t^3 & t^2 & t & 1
\end{bmatrix}
$$

the control points vector:
$$
P = \begin{bmatrix}
P_0\\
P_1\\
P_2\\
P_3
\end{bmatrix}
$$

and finally the Catmull-Rom matrix:
$$
M_{CR} = {1\over2}\begin{bmatrix}
-1 & 3 & -3 & 1 \\
2 & -5 & 4 & -1 \\
-1 & 0 & 1 & 0 \\
0 & 2 & 0 & 0 \\
\end{bmatrix}
$$

The path time-based formula is:
$$
p(t) = T\cdot M_{CR}\cdot P
$$

This vectorized version makes our performance faster, since we can avoid directly using (nested) for loops, which is essential to our purpose.
To achieve this, we implemented `vec.h` to generalize vec3 to support N dimension and `matrix.h` to support NxM matrices, based on `vec` class as well.

#### `vec.h` implementation

We used C++17 [**fold expressions**](https://en.cppreference.com/cpp/language/fold) extensively to ensure lightning-fast performance.
Being a generalized version of `vec3`, we are going to analyze the `vec` class for differences.

1. Definition

```C++
template <std::size_t N>
class vec {
public:
  float e[N]{};
  ...
```

Adding a template unsigned size parameter makes array size allocation possible at compile time. To declare a, say, 7D vector: `vec<7> vec7{};`.

2. Operations

The class gives a definition for vector sum and vector-by-vector and scalar-by-vector product, as well as scalar product.
In particular, vector sums and vector-by-vector products have a shallow broadcasting mechanism, by always tossing away the excess elements from the larger matrix.

```C++
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
```

3. Unrolling

We could simply use for loops; however, since this is a graphical engine, performance is a key target. Although aware of the fact that this particular course is focused on offline rendering, we decided to yield a high-performance library anyway.

Loop unrolling is the practice of expanding a loop into a sequential chain of instructions to eliminate loop control overhead. In modern C++, we can achieve compile-time unrolling using C++17 Fold Expressions combined with std::index_sequence.
In the scalar product example, the compiler expands the parameter pack Is... into a single, flat expression at compile-time:

```C++
template <std::size_t M, std::size_t... Is>
float unrolled_dot(const vec<M>& v, std::index_sequence<Is...>) const {
    return ((e[Is] * v.e[Is]) + ...);
}
```

It functions identically to a MapReduce pattern:
- Map: (e[Is] * v.e[Is]) multiplies corresponding elements for each index;
- Reduce: The + ... syntax (a right fold) sums all the multiplied results together into a single scalar value without ever generating a runtime loop.

4. Inline functions

Just like `vec3`, `vec` gets its set of inline functions by exploiting self-assignment overridden operators.
For instance, defining inline vector product:
```C++
template <std::size_t N>
inline vec<N> operator*(vec<N> u, const vec<N>& v) {
  return u *= v;
}
```
Is just as convenient as reusing the `*=` overridden operator.

#### `matrix.h` implementation

`matrix` class takes advantage of the same paradigm explained above.
We regarded matrix as a simple list of row vectors, so that an $R\times C$ matrix is actually an array of size R of `vec<C>`.

```C++
template <std::size_t R, std::size_t C>
class matrix
{
public:
  vec<C> rows[R];
  ...
```

The implementation follows the same fashion as `vec`, using helpers for unrolling and overriding operators.
We provided the following operations:

- Sum of matrices of same size
- Scalar-by-matrix product
- Vector-by-matrix product and matrix-by-vector product
- Matrix-by-Matrix product
- Hadamard product (matrices of same size only)

Since most of these operations are reduced to vector operations, most of the complexity was managed by `vec` class in the first place.

Note: Catmull-Rom needs a **row vector** by matrix product. Since our matrix is naturally structured as an array of row vectors, implementing this was straightforward; a row vector of $N$ elements can be simply passed as a `vec<N>` to scale and sum the matrix rows. However, looking at our vector-by-matrix definition:

```C++

  //note: the accessor is private since is used by a public inline function
  template <std::size_t K>
  friend matrix<R, K> operator*(const matrix &A, const matrix<C, K> &B) 
  {
    return A.unrolled_product(B, std::make_index_sequence<R>{});
  }

  ...

  private:

  template <std::size_t... Is>
  vec<C> unrolled_vec_product(const vec<R> &v, std::index_sequence<Is...>) const
  {
    return ((v.e[Is] * rows[Is]) + ...);
  }
```

this already performs a linear combination of the matrix's rows, so that doing:

1. `matrix<1,4> * matrix<4,5>`
2. `vec<4> * matrix<4,5>`

is equivalent.

### b. Implementation of the spline.

We decoupled the computation of the spline into a separate `spline.h` header file.
The key idea is giving the renderer a list of `point3`s, which we defined in a `point3_list.h` header, as points where we'd like our camera to be at specific times.
While keeping the convenience of using a `point3`, used everywhere else in the codebase, we also need a way to convert a `point3` into a `vec<3>` for vectorized computation of Catmull-Rom spline. We have provided these conversion methods in both `vec.h` and `matrix.h`:
```C++
//vec.h

inline vec<3> fromVec3(vec3 v3) {
    return vec<3>{v3.x(), v3.y(), v3.z()};
}

inline vec3 toVec3(vec<3> v) {
    return vec3(v[0], v[1], v[2]);
}

//matrix.h

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
```

To calculate the spline, we used a wrapper function, which prepares the control points vector to obtain a set that is consistent for obtaining a closed curve for our camera.

```C++
//note: path is an alias for point3_list
inline path catmull_rom_interpolate(const point3_list& controlSet, const float dt) {
   int size = (int)controlSet.points.size();

   point3_list positions{ controlSet.points.back() };
   positions.add(controlSet);
   positions.add(controlSet.points[0]);
   positions.add(controlSet.points[1 % size]);

   ...
```

Then we declare the Catmull-Rom matrix:

```C++
matrix<4,4> M_CR = 0.5f * matrix<4, 4>{{{-1, 3, -3, 1}, { 2, -5, 4, -1 }, { -1, 0, 1, 0 }, { 0, 2, 0, 0 }}};
```

We need to calculate the time vector as well. To have a complete trajectory, our time vector needs to be interpolated by incrementing time by small amounts.
By tweaking the `dt` parameter, we can decide the trajectory density. A value closer to $\epsilon$ means our spline is finer, while a value closer to 1 means it's coarser.

Everything is now ready for the closed spline to be calculated.
Since we are taking into account more than 4 points, we can use a moving window of four points at a time to generate a first spline. Then, by sliding the window forward by one point, we calculate the spline again, adding the result to our resulting set of points (path), then sliding the window forward by one position, and so on.

This happens all at once in place, inside a for loop:

```C++
    ...
    int segments = (int)positions.points.size() - 3;
    for (int i = 0; i < segments; i++) {
        matrix<4, 3> controlSubset{ fromVec3(positions.points[i]), fromVec3(positions.points[i + 1]), fromVec3(positions.points[i + 2]), fromVec3(positions.points[i + 3]) };
        for (float t = 0.0f; t < 1.0f; t += dt) {
            vec<4> T{ t * t * t, t * t, t, 1 };
            pathPoints.add(toVec3(T * M_CR * controlSubset));
        }
        pathPoints.add(toVec3(T * M_CR * controlSubset));
    }
    ...
```

### c. Moving the camera and generating the numbered frames

...

### 3. Extra Achievements

Add juicy docs.
