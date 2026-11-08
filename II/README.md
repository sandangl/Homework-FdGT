# Second Homework

## Introduction

The codebase (`./src/`) is based on the 10th lecture version of the graphics engine, available [here](https://informatica.unibas.it/moodle/mod/resource/view.php?id=21179). The starting engine integrates TinyBVH acceleration for triangle meshes, Phong illumination, shadow ray evaluation, affine transformation instances, OBJ mesh loading, and texture mapping.

This project implements the following assignments and advanced rendering techniques:

- **Simple and Smooth Spotlight** (Homework 1 - Easy)
- **Multiple Light Sources & Extended Light / Soft Shadows** (Homework 3 - Medium / High)
- **Toon Shading / Cel Shading** (Homework 4 - Easy, Extra)
- **Sky & Environment Texture Mapping with Solar Direction Estimation** (Homework 9 - Easy, Extra)
- **Primitive Texture Mapping & Wrapping/Repetition** (Homework 8 - Easy / Medium)
- **Motion Blur & Moving Instances** (Homework 5 - Medium)
- **Modular Scene Architecture & Spotlight Unit Test Suite** (Engineering / Extra)
- **Animated Short Film** (Homework 10 - Final Homework)

---

## 1. Simple and Smooth Spotlight

A spotlight is a directional light emitter that confines its illumination to a conical beam centered along an orientation vector $\mathbf{s}$. It is parameterized by:

- Light position: $\mathbf{p}_s \in \mathbb{R}^3$
- Unit aim direction vector: $\mathbf{s} \in \mathbb{S}^2$
- Outer cone aperture half-angle: $\theta_0$
- Inner cone half-angle: $\theta_1 = \text{clamp}(\text{smoothness}, 0, 1) \cdot \theta_0$
- Decaying speed exponent: $\gamma$ (default $\gamma = 30.0$)

### a. Angular Inclusion Test

For an arbitrary surface point $\mathbf{p}$, we compute the unit vector pointing from the light emitter towards $\mathbf{p}$:

$$
\mathbf{l} = \frac{\mathbf{p} - \mathbf{p}_s}{\|\mathbf{p} - \mathbf{p}_s\|}
$$

The angle $\theta$ between $\mathbf{l}$ and the central cone axis $\mathbf{s}$ determines whether $\mathbf{p}$ lies inside the illuminated cone. Rather than computing expensive inverse trigonometric functions ($\arccos$), we evaluate the dot product directly:

$$
\cos\theta = \mathbf{l} \cdot \mathbf{s}
$$

Because cosine decreases monotonically on $[0, \pi]$, an angle $\theta$ is inside the cone if and only if:

$$
\cos\theta > \cos\theta_0
$$

Points with $\cos\theta \le \cos\theta_0$ fall strictly outside the cone and receive only ambient illumination.

### b. Smooth Falloff ($f_{\text{spot}}$) and Boundary Stability

To avoid harsh, unnatural spotlight edges, the spotlight implements a dual-cone transition. The attenuation factor $f_{\text{spot}} \in [0, 1]$ modulates diffuse and specular reflections across three regions:

1. **Inner Cone** ($\cos\theta > \cos\theta_1$):
   The point resides in the core illuminated cone. The light decays as a power-law function of the cosine:
   $$
   t = (\cos\theta)^\gamma, \quad f_{\text{spot}} = t
   $$

2. **Outer Transition Zone** ($\cos\theta_0 < \cos\theta \le \cos\theta_1$):
   The point lies in the penumbra between the inner and outer cones. An exponential decay accelerates the falloff towards the outer boundary:
   $$
   f_{\text{spot}} = t^2 = (\cos\theta)^{2\gamma}
   $$

3. **Outside Cone** ($\cos\theta \le \cos\theta_0$):
   $$
   f_{\text{spot}} = 0
   $$

```C++
// light.h
class spotlight : public light {
public:
    point3 p_s;
    vec3 s;
    float theta_0;
    float theta_1;
    float decaying_speed;

    spotlight(point3 p_s, vec3 direction, float aperture, float smoothness,
              color a, color d, color s_col, float speed = 30.0f)
    {
        position = p_s;
        this->p_s = p_s;
        s = unit_vector(direction);
        theta_0 = aperture;
        theta_1 = tinybvh::tinybvh_clamp(smoothness, 0.0f, 1.0f) * theta_0;
        decaying_speed = speed;
        ambient = a;
        diffuse = d;
        specular = s_col;
    }

private:
    float calculate_fspot(vec3 l) override
    {
        float cos_theta = dot(l, s);
        float cos_1 = cos(degrees_to_radians(theta_1));
        float cos_0 = cos(degrees_to_radians(theta_0));
        float t = pow(cos_theta, decaying_speed);

        if (cos_theta > cos_1) {
            return t;       // inner cone: linear/power decay
        }
        if (cos_theta > cos_0) {
            return t * t;   // outer cone: exponential penumbra decay
        }
        return 0.0f;
    }
};
```

#### Numerical Stability and Bugfixes

During initial development, the transition factor was evaluated as:

$$
t = \frac{\cos\theta - \cos\theta_0}{\cos\theta_1 - \cos\theta_0}
$$

When $\text{smoothness} = 1.0$ (matching inner and outer cone angles, $\theta_1 = \theta_0$), the denominator $(\cos\theta_1 - \cos\theta_0)$ evaluated to zero, resulting in `NaN` and numerical explosion. Furthermore, when $\theta_1 \approx \theta_0$, $t$ exhibited steep gradients that caused visible ring artifacts. 

The revised implementation directly clamps $\theta_1 \le \theta_0$ and uses monotonic power-law decay $t = (\cos\theta)^\gamma$ with quadratic penumbra attenuation $t^2$, guaranteeing:
- Exact hard cutoffs when $\theta_1 = \theta_0$ without division-by-zero.
- Strict circular symmetry around $\mathbf{s}$.
- Distance invariance: $f_{\text{spot}}$ modulates angular spread only, keeping distance attenuation cleanly orthogonal.

### c. Deliverables

| Hard Edge (`1_spotlight_netto.bmp`) | Smooth Edge (`1_spotlight_sfumato.bmp`) |
|:---:|:---:|
| ![Hard Edge Spotlight](./deliverables/1_spotlight_netto.bmp) | ![Smooth Edge Spotlight](./deliverables/1_spotlight_sfumato.bmp) |
| $\theta = 22^\circ$, $\text{smoothness} = 1.0$ | $\theta_i = 12^\circ, \theta_o = 24^\circ$, $\text{smoothness} = 0.5$ |

---

## 2. Multiple Light Sources & Extended Light (Soft Shadows)

In earlier iterations of the renderer, `ray_color` received a single hard-coded `point_light`. We refactored the lighting subsystem into an extensible, object-oriented hierarchy supporting heterogeneous light sources in a unified container.

### a. Polymorphic Light Hierarchy

The base class `light` provides virtual methods that encapsulate light-specific geometry and attenuation:

```C++
// light.h
class light {
public:
    point3 position;
    color ambient;
    color diffuse;
    color specular;

    virtual ~light() = default;

    virtual vec3 get_direction(const point3& p) const {
        return unit_vector(position - p);
    }

    virtual float get_distance(const point3& p) const {
        return (position - p).length();
    }

    virtual float calculate_light_interaction(point3 p) {
        vec3 l = unit_vector(p - position);
        return calculate_fspot(l);
    }

private:
    virtual float calculate_fspot(vec3 l) {
        return 1.0f;
    }
};
```

The system provides concrete implementations for all primary lighting types:

1. **`point_light`**: Omnidirectional point emitter with radial distance attenuation and $f_{\text{spot}} = 1.0$.
2. **`directional_light`**: Distant light source (e.g. sunlight). The incident direction $\mathbf{L}$ is constant across the entire scene, and distance to source is $\infty$, creating strictly parallel shadow rays:
   ```C++
   class directional_light : public light {
   public:
       vec3 dir;
       directional_light(vec3 direction_to_light, color a, color d, color s) {
           dir = unit_vector(direction_to_light);
           ambient = a; diffuse = d; specular = s;
       }
       vec3 get_direction(const point3& p) const override { return dir; }
       float get_distance(const point3& p) const override { return infinity; }
       float calculate_light_interaction(point3 p) override { return 1.0f; }
   };
   ```
3. **`spotlight`**: Conical emitter overriding `calculate_fspot()` to evaluate angular penumbra falloff.
4. **`ext_light`**: Area light source that dynamically generates Monte Carlo samples across a planar quadrilateral.

### b. Normalized Multi-Source Accumulation

When multiple light sources illuminate a point, simply summing unattenuated contributions quickly saturates pixel channels beyond $1.0$, causing harsh white clamping and destroying surface color definition.

To maintain correct dynamic range, `camera::ray_color` normalizes contributions across the active light count $N_{\text{sources}} = |\text{lights}|$:

$$
\mathbf{I}(\mathbf{p}) = \frac{1}{N_{\text{sources}}} \sum_{i=1}^{N_{\text{sources}}} \left[ V_i \cdot \mathbf{I}_{\text{illum}}(\text{light}_i, \mathbf{p}) + (1 - V_i) \cdot \mathbf{I}_{\text{ambient}}(\text{light}_i, \mathbf{p}) \right]
$$

where $V_i \in \{0, 1\}$ represents the binary visibility from the shadow ray:

$$
\mathbf{r}_{\text{shadow}} = \text{ray}(\mathbf{p}, \mathbf{L}_i), \quad t \in [0.01, d_{\text{light}}]
$$

```C++
// camera.h - Multi-light integration loop in ray_color()
color pixel_color(0.0f, 0.0f, 0.0f);
int num_sources = (int) lights.sources.size();

for (light* source : lights.sources)
{
    vec3 l = source->get_direction(rec.p);
    float closest_light = source->get_distance(rec.p);
    ray shadow_ray(rec.p, l);

    if (world.hit_shadow(shadow_ray, interval(0.01f, closest_light))) {
        pixel_color += ambient_shading(*source, rec) / num_sources;
    } else {
        if (rec.m == nullptr || rec.m->is_toon) {
            pixel_color += toon_shading(*source, rec, camera_center, K) / num_sources;
        } else {
            pixel_color += phong_shading(*source, rec, camera_center) / num_sources;
        }
    }
}
return pixel_color;
```

### c. Extended Light Sources & Soft Shadows (Monte Carlo Area Light)

Physical luminaires have non-zero surface area. Points in the penumbra region observe only a portion of the emitter, resulting in gradual, soft shadows rather than razor-sharp silhouettes.

We implemented `ext_light` and `point_ext_light`, defined by a base corner vertex $\mathbf{Q} \in \mathbb{R}^3$ and two spanning tangent vectors $\mathbf{u}, \mathbf{v} \in \mathbb{R}^3$:

$$
\mathbf{p}(\xi_1, \xi_2) = \mathbf{Q} + \xi_1 \mathbf{u} + \xi_2 \mathbf{v}, \quad \xi_1, \xi_2 \sim \mathcal{U}(0, 1)
$$

```C++
// ext_light.h
class point_ext_light : public ext_light {
public:
    point_ext_light(const point3& Q, const vec3& u, const vec3& v,
                    color a, color d, color s, int num_samples = 50)
        : ext_light(Q, u, v, a, d, s, num_samples) {}

    light_list getSources() override {
        light_list lights;
        for (int i = 0; i < num_samples; i++) {
            point3 sample_pos = Q + random_float() * u + random_float() * v;
            point_light* sample = new point_light(sample_pos, ambient, diffuse, specular);
            lights.add(sample);
        }
        return lights;
    }
};
```

To integrate extended lights without modifying the core renderer, `light_list` provides overloaded `add()` operators that automatically unpack the sample collection into the unified light list:

```C++
inline void light_list::add(ext_light* extended) {
    if (extended) add(extended->getSources());
}
```

By casting shadow rays to $N$ independently sampled points across the quadrilateral emitter (e.g. $N = 256$), the visibility fraction:

$$
V(\mathbf{p}) = \frac{1}{N} \sum_{k=1}^N V_k(\mathbf{p})
$$

yields photorealistic penumbra gradients around occluders.

### d. Scene Setups & Demonstrations

In `scenes.h`, we constructed multiple pre-configured scenes demonstrating each lighting capability:

- `create_scene_1_spotlight_only()`: Stanford Bunny illuminated from above by a single focused spotlight ($\theta_0 = 20^\circ$).
- `create_scene_2_no_wall_all_lights()`: Simultaneous co-existence of `point_light`, `directional_light`, `spotlight`, and `point_ext_light` ($N = 256$ samples).
- `create_scene_3_three_spotlights()`: Three distinct spotlights (Red, White, Blue) intersecting at a single focal point, producing colored overlapping shadows with complementary hues.
- `create_scene_4_extended_light_only()`: Area light soft shadows cast by an obstruction sphere onto the Stanford Bunny and ground plane.
- `create_scene_5_all_lights()`: Combined multi-emitter environment with complex shadow interactions.

#### Multi-Light Demonstrations

| Colored Point Lights (`3_tre_luci_colorate.bmp`) | Three Spotlights (`3_tre_spotlights.bmp`) |
|:---:|:---:|
| ![Three Colored Lights](./deliverables/3_tre_luci_colorate.bmp) | ![Three Spotlights](./deliverables/3_tre_spotlights.bmp) |
| RGB point lights producing CMY shadows | Three colored spotlights converging on scene |

#### Extended Light & Soft Shadows ($N$ Monte Carlo Samples)

Measurements on $700 \times 393$ resolution (multi-threaded PPL):

| Point Light Baseline (`3_ombre_morbide_puntiforme.bmp`) | Area Light $N=1$ (`3_ombre_morbide_N1.bmp`) |
|:---:|:---:|
| ![Point Light Baseline](./deliverables/3_ombre_morbide_puntiforme.bmp) | ![Area Light N=1](./deliverables/3_ombre_morbide_N1.bmp) |
| Hard shadow (9.775 s) | Stochastic noise (7.204 s) |

| Area Light $N=16$ (`3_ombre_morbide_N16.bmp`) | Area Light $N=256$ (`3_ombre_morbide_N256.bmp`) |
|:---:|:---:|
| ![Area Light N=16](./deliverables/3_ombre_morbide_N16.bmp) | ![Area Light N=256](./deliverables/3_ombre_morbide_N256.bmp) |
| Reduced noise (5.814 s) | Smooth penumbra (8.088 s) |

---

## 3. Toon Shading

Toon shading (non-photorealistic rendering) creates stylized, animated cartoon aesthetics by quantizing continuous diffuse gradients into discrete color steps, adding ink-like silhouette outlines, and replacing specular highlights with crisp disks.

### a. Quantized Diffuse Reflection ($K$ Levels)

In standard Lambertian shading, diffuse reflection scales smoothly with $d = \max(\mathbf{n} \cdot \mathbf{l}, 0)$. In toon shading, the parameter $K \in \mathbb{N}^+$ defines the number of discrete luminance bands:

$$
d = \frac{\lceil K \cdot \max(\mathbf{n} \cdot \mathbf{l}, 0) \rceil}{K}, \quad d \in \left\{0, \frac{1}{K}, \frac{2}{K}, \dots, 1\right\}
$$

- For unlit regions where $\mathbf{n} \cdot \mathbf{l} \le 0$, $d = 0$, leaving only ambient illumination.
- For illuminated regions, $d$ snaps to the ceiling step:
  - $K = 1$: Single binary illumination tone.
  - $K = 2$: Two distinct light tiers.
  - $K = 3$ or $K = 5$: Classic comic book cel-shading.
  - As $K \to \infty$: Converges back to smooth Phong shading.

### b. Stylized Specular Blotches

Smooth exponential specular highlights $(\max(\mathbf{v} \cdot \mathbf{r}, 0))^\alpha$ are replaced by a sharp thresholded disc:

$$
f_{\text{spec}} = \begin{cases}
1.0 & \text{if } |\mathbf{v} \cdot \hat{\mathbf{r}}| > 0.95 \\
0.0 & \text{otherwise}
\end{cases}
$$

### c. Silhouette Outlines and Material-Level Decoupling

A silhouette edge occurs where the surface curves away from the observer and is viewed edge-on (grazing angle). In this regime, the surface normal $\mathbf{n}$ is nearly orthogonal to the view vector $\mathbf{v}$:

$$
|\mathbf{n} \cdot \mathbf{v}| < \varepsilon \implies \mathbf{I}_{\text{pixel}} = (0, 0, 0)
$$

```C++
// shader.h
color toon_shading(light& light, hit_record& hr, point3& camera_pos, const int K = 3, const float epsilon = 0.26f) {
    int k_val = (hr.m != nullptr && hr.m->K > 0) ? hr.m->K : K;
    float eps = (hr.m != nullptr) ? hr.m->epsilon : epsilon;

    // Silhouette outline: if surface is viewed edge-on, draw black contour
    vec3 V = unit_vector(camera_pos - hr.p);
    float NdotV = std::fabs(dot(V, hr.normal));
    if (NdotV < eps) {
        return color(0.0f, 0.0f, 0.0f);
    }

    color ambient = hr.m->ka * light.ambient;
    color diffuse(0.0f, 0.0f, 0.0f);
    color specular(0.0f, 0.0f, 0.0f);

    vec3 L = light.get_direction(hr.p);
    float LDotN = std::max(dot(L, hr.normal), 0.0f);

    if (LDotN > 0.0f) {
        color kd = (hr.m->tex != nullptr) ? hr.m->tex->value(hr.u, hr.v, hr.p) : hr.m->kd;
        float d = std::ceil((float)k_val * LDotN) / ((float)k_val);
        diffuse = kd * light.diffuse * d;

        vec3 R = reflect(L, hr.normal);
        float VDotR = std::abs(dot(V, unit_vector(R)));

        float spec_factor = (VDotR > 0.95f) ? 1.0f : 0.0f;
        specular = hr.m->ks * light.specular * spec_factor;

        float f = light.calculate_light_interaction(hr.p);
        return ambient + f * (diffuse + specular);
    }

    return ambient;
}
```

#### Why $|\mathbf{n} \cdot \mathbf{v}|$ Fails on Planar Surfaces and How We Fixed It

The course slides ask: *Where does the contour calculated with $|\mathbf{n} \cdot \mathbf{v}|$ fail?*

On flat horizontal ground planes viewed from an elevated camera angle towards the horizon, the surface normal is $\mathbf{n} = (0, 1, 0)$. As rays approach the distant horizon, the view vector $\mathbf{v}$ becomes nearly parallel to the ground, so $\mathbf{n} \cdot \mathbf{v} \to 0$. A global $|\mathbf{n} \cdot \mathbf{v}| < \varepsilon$ test incorrectly flags the entire horizon as a silhouette, drawing an unsightly thick black band across the floor.

To solve this, we decoupled toon shading parameters from the camera shader into `material`:

```C++
// material.h
class material {
public:
    color ka, kd, ks;
    float alpha;

    bool is_toon = false;
    int K = 3;
    float epsilon = 0.26f;
    texture* tex = nullptr;
    ...
};
```

In `create_toon_scene()`, curved geometry (Stanford Bunny, spheres) sets `mat->is_toon = true`, enabling crisp contours and discrete stepped shading, while the horizontal floor plane sets `floor_mat->is_toon = false`, keeping the ground horizon clean and free of dark artifacts.

### d. Deliverables

| Phong Baseline (`4_toon_phong.bmp`) | Toon $K=2$ (`4_toon_K2.bmp`) |
|:---:|:---:|
| ![Phong Baseline](./deliverables/4_toon_phong.bmp) | ![Toon K=2](./deliverables/4_toon_K2.bmp) |
| Continuous Phong reference | 2 quantization levels, specular blotch and silhouette |

| Toon $K=3$ (`4_toon_K3.bmp`) | Toon $K=5$ (`4_toon_K5.bmp`) |
|:---:|:---:|
| ![Toon K=3](./deliverables/4_toon_K3.bmp) | ![Toon K=5](./deliverables/4_toon_K5.bmp) |
| 3 quantization levels | 5 quantization levels |

---

## 4. Sky & Environment Texture Mapping

To replace the artificial vertical background gradient with photorealistic surroundings, we developed `environment_texture`, an equirectangular 360° panoramic background mapper coupled with automated solar direction extraction.

### a. Directional Equirectangular Projection

When a primary ray fails to intersect any scene geometry ($!world.hit$), its normalized direction vector $\mathbf{d} = (d_x, d_y, d_z)$ is mapped into spherical coordinates $(\theta, \phi)$ and normalized texture coordinates $(u, v) \in [0, 1]^2$:

$$
\theta = \arccos(\text{clamp}(d_y, -1, 1)), \quad \phi = \text{atan2}(d_x, -d_z)
$$

$$
u = \frac{1}{2} + \frac{\phi}{2\pi}, \quad v = 1 - \frac{\theta}{\pi}
$$

#### Inside-Out View Inversion

Standard spherical texture coordinates assume the viewer is looking at the exterior of the sphere. Looking from the inside causes the panoramic image to be mirrored horizontally, rendering landmarks and text backwards. Setting $\phi = \text{atan2}(d_x, -d_z)$ corrects this parity reversal, ensuring the panoramic sky displays with correct left-to-right orientation.

### b. Bilinear Filtering with Horizontal Toroidal Wrap

Nearest-neighbor sampling of high-resolution equirectangular textures produces pixelated artifacts and harsh discontinuities at the $360^\circ$ seam ($u = 0 \leftrightarrow u = 1$).

We implemented bilinear filtering with horizontal wrap-around ($u \pmod 1$) and vertical clamping:

```C++
// texture.h - image_texture::value()
u = u - std::floor(u); // Toroidal wrap-around across [0, 1)
v = std::clamp(v, 0.0f, 1.0f);

float x = u * float(imageWidth) - 0.5f;
float y = (1.0f - v) * float(imageHeight) - 0.5f;

int x0 = int(std::floor(x));
int y0 = int(std::floor(y));
int x1 = x0 + 1;
int y1 = y0 + 1;

float fx = x - float(x0);
float fy = y - float(y0);

// Seamless boundary handling
x0 = (x0 % imageWidth + imageWidth) % imageWidth;
x1 = (x1 % imageWidth + imageWidth) % imageWidth;
y0 = std::clamp(y0, 0, imageHeight - 1);
y1 = std::clamp(y1, 0, imageHeight - 1);

// Four-tap bilinear interpolation
color c0 = (1.0f - fx) * c00 + fx * c10;
color c1 = (1.0f - fx) * c01 + fx * c11;
return (1.0f - fy) * c0 + fy * c1;
```

### c. Panoramic View Framing

Standard equirectangular maps span $360^\circ \times 180^\circ$. When viewed through a narrow field-of-view camera ($\text{vfov} = 20^\circ$), only a small fraction of the image is visible. To frame the background panorama pleasingly behind foreground objects, `environment_texture` exposes:

- `zoom`: Scales angular spread ($> 1.0$ displays a wider panorama in narrow FOV without altering object perspective).
- `rotation_y`: Rotates the sky azimuthally around the Y axis.
- `pitch_offset`: Adjusts the vertical horizon line.

### d. Solar Direction Estimation via Stochastic Gradient Ascent (SGA)

For shadows to appear natural against an environment background, scene lighting must align with the primary light source visible in the sky image (the sun).

Instead of an exhaustive $O(W \times H)$ search, `environment_texture` utilizes multi-restart Stochastic Gradient Ascent (SGA):

1. **Restarts**: 20 random starting coordinates $(x_r, y_r)$ sampled uniformly across the image.
2. **Gradient Ascent**: From each start, inspect the 8-neighborhood and step towards the texel with maximum PAL luminance:
   $$
   Y = 0.222 R + 0.707 G + 0.071 B
   $$
3. **Convergence**: Stop when all 8 neighbors have lower or equal luminance. The highest peak among all restarts identifies the solar disc coordinates $(x_{\text{sun}}, y_{\text{sun}})$.
4. **Direction Vector**: Convert $(x_{\text{sun}}, y_{\text{sun}})$ to spherical angles and build the 3D unit direction:
   $$
   \mathbf{s}_{\text{sun}} = (\sin\theta \sin\phi, \cos\theta, -\sin\theta \cos\phi)
   $$

```C++
// texture.h - estimate_sun_direction()
float u = (best_overall_x + 0.5f) / float(imageWidth);
float v = 1.0f - (best_overall_y + 0.5f) / float(imageHeight);
float theta = (1.0f - v) * pi;
float phi = (u - 0.5f) * 2.0f * pi + rotation_y;
float sin_theta = std::sin(theta);

sun_direction = unit_vector(vec3(sin_theta * std::sin(phi), std::cos(theta), -sin_theta * std::cos(phi)));
```

#### Ground Truth Validation & Angular Error

To evaluate SGA accuracy, `compute_ground_truth_sun()` computes the weighted barycentric centroid of all pixels exceeding $98\%$ of the maximum luminance ($L \ge 0.98 L_{\max}$):

$$
\bar{x} = \frac{\sum_i x_i w_i}{\sum_i w_i}, \quad \bar{y} = \frac{\sum_i y_i w_i}{\sum_i w_i}, \quad w_i = L_i - L_{\min}
$$

The angular error between the estimated sun vector $\hat{\mathbf{d}}_{\text{est}}$ and the ground-truth centroid vector $\hat{\mathbf{d}}_{\text{true}}$ is calculated in degrees:

$$
\Delta\theta = \arccos(\text{clamp}(\hat{\mathbf{d}}_{\text{est}} \cdot \hat{\mathbf{d}}_{\text{true}}, -1, 1)) \cdot \frac{180^\circ}{\pi}
$$

When running `create_sky_scene()`, the engine automatically outputs the validation report:

```text
Sky Texture: ../models/canyon_sky.png (2048x1024)
SGA Sun Pixel: (1603, 318)
True Sun Centroid: (1602.4, 318.1)
Sun Direction Angular Error: 0.089 deg
```

The resulting `directional_light` casts crisp shadows from the Stanford Bunny and spheres that correspond to the position of the sun in the sky.

### e. Deliverables

| Procedural Gradient Sky (`9_cielo_gradiente.bmp`) | Equirectangular Sky Texture (`9_cielo_texture.bmp`) |
|:---:|:---:|
| ![Procedural Gradient Sky](./deliverables/9_cielo_gradiente.bmp) | ![Equirectangular Sky Texture](./deliverables/9_cielo_texture.bmp) |
| Vertical gradient with point light | Equirectangular map with SGA-aligned directional sunlight |

---

## 5. Primitive Texture Mapping & Wrapping/Repetition

While mesh texturing relies on per-vertex UV coordinates exported by DCC software and stored in OBJ files, geometric primitives (sphere, cylinder with planar caps, quad, and axis-aligned box) require analytic parameterizations computed directly in their intersection routines (`hit()`).

### a. Object-Space Parameterization Invariance

All primitive texture coordinates $(u, v) \in [0, 1]^2$ are computed in **object space** prior to world transformation. Because affine transformations (translations, rotations, non-uniform scaling) are applied to the ray via `instance::hit()` using the inverse matrix $\mathbf{M}^{-1}$, the surface coordinates remain rigidly attached to the geometry. The texture moves, rotates, and scales with the instance without sliding or distortion.

### b. Sphere (`sphere.h`)

For a sphere centered at the origin, the outward unit normal $\mathbf{n} = \mathbf{p} / R$ is converted to spherical polar coordinates:

$$
\theta = \arccos(\text{clamp}(n_y, -1, 1)), \quad \phi = \text{atan2}(n_z, n_x)
$$

The angular domain $\theta \in [0, \pi]$ and $\phi \in [-\pi, \pi]$ maps to normalized texture coordinates:

$$
u = \frac{\phi + \pi}{2\pi}, \quad v = 1 - \frac{\theta}{\pi}
$$

```C++
// sphere.h - hit()
vec3 outward_normal = (rec.p - center) / radius;
rec.set_face_normal(r, outward_normal);

float theta = std::acos(clamp(outward_normal.y(), -1.0f, 1.0f));
float phi = std::atan2(outward_normal.z(), outward_normal.x());
rec.u = (phi + pi) / (2.0f * pi);
rec.v = 1.0f - theta / pi;
```

#### Poles and Seams
- **Poles**: Singularities occur at the apex and base ($y = \pm R$), where all longitudinal meridians converge to a single point. Here, $\theta = 0$ or $\theta = \pi$, collapsing $u$ to a degenerate line across the top/bottom texel rows.
- **Seam**: A vertical discontinuity exists along the meridian opposite the primary view ($\phi = \pm\pi$), where $u$ abruptly transitions from $1.0$ to $0.0$. Bilinear samplers must wrap toroidally across this boundary.

### c. Capped Cylinder (`cylinder.h`)

A finite cylinder bounded between $y_{\min}$ and $y_{\max}$ consists of three distinct geometric components: a curved lateral mantle and two planar circular caps.

1. **Curved Lateral Mantle** ($y_{\min} < y < y_{\max}$):
   The azimuthal angle around the central $Y$-axis determines $u$, while height along the cylinder axis determines $v$:
   $$
   u = \frac{1}{2} + \frac{\text{atan2}(x, z)}{2\pi}, \quad v = \frac{y - y_{\min}}{y_{\max} - y_{\min}}
   $$

2. **Top Cap** ($y = y_{\max}$, $\mathbf{n} = (0, 1, 0)$):
   Orthographic projection of the planar disc coordinates $(x, z) \in [-r, r]^2$:
   $$
   u = \frac{1}{2} + \frac{x}{2r}, \quad v = \frac{1}{2} + \frac{z}{2r}
   $$

3. **Bottom Cap** ($y = y_{\min}$, $\mathbf{n} = (0, -1, 0)$) & $x$-Axis Inversion:
   $$
   u = \frac{1}{2} - \frac{x}{2r}, \quad v = \frac{1}{2} + \frac{z}{2r}
   $$

#### Mathematical Justification for Bottom Cap Inversion
The outward normal of the bottom cap points in the $-Y$ direction. When an observer looks at the underside disc from below (facing upward along $+Y$), the positive $X$-axis points to their right. Without negating $x$, the projected horizontal coordinate would mirror the image horizontally compared to viewing from above. Inverting the sign ($0.5 - x / 2r$) ensures alphanumeric patterns (such as diagnostic test grids) read correctly from left to right from both top and bottom vantage points.

### d. Planar Primitives: Quad and Axis-Aligned Box (`geometry.h`)

#### Quadrilateral (`quad`)
Given a planar quad defined by base corner $\mathbf{Q}$ and spanning vectors $\mathbf{u}_{\text{vec}}, \mathbf{v}_{\text{vec}}$, the plane constant $D = \mathbf{n} \cdot \mathbf{Q}$ and planar projection vector $\mathbf{w} = \mathbf{n} / (\mathbf{n} \cdot \mathbf{n})$ yield bilinear coordinates directly from cross products:

$$
\alpha = \mathbf{w} \cdot ((\mathbf{p} - \mathbf{Q}) \times \mathbf{v}_{\text{vec}}), \quad \beta = \mathbf{w} \cdot (\mathbf{u}_{\text{vec}} \times (\mathbf{p} - \mathbf{Q}))
$$

If $\alpha, \beta \in [0, 1]$, the ray hits the quadrilateral with $(u, v) = (\alpha, \beta)$.

#### Axis-Aligned Box (`box`)
An axis-aligned box between $\mathbf{p}_{\min}$ and $\mathbf{p}_{\max}$ classifies the intersected face using the dominant component of the hit normal $\mathbf{n}$:

- **Vertical Faces** ($\pm X, \pm Z$):
  To preserve uniform vertical alignment and prevent inverted textures, all vertical faces project height identically:
  $$
  v = \frac{y - y_{\min}}{y_{\max} - y_{\min}}
  $$
  The horizontal coordinate $u$ projects the remaining orthogonal axis with appropriate signs to maintain clockwise continuity around the box perimeter.
- **Top / Bottom Faces** ($\pm Y$):
  Planar projection of $(x, z)$ across $[\Delta x, \Delta z]$.
- **Poles & Seams**: Boxes possess no polar singularities. Seams correspond exactly to the 12 geometric edges where the projection plane abruptly changes orientation.

### e. Sampler Upgrade: Repetition Factors and Fractional Wrapping (`texture.h`)

Standard texture samplers often clamp coordinates to $[0, 1]$, causing unsightly smeared borders when coordinates extend beyond the unit square. We upgraded `image_texture` with independent repetition scaling $(s_u, s_v)$ and periodic fractional wrapping:

$$
u' = (u \cdot s_u) - \lfloor u \cdot s_u \rfloor, \quad v' = (v \cdot s_v) - \lfloor v \cdot s_v \rfloor
$$

```C++
// texture.h - image_texture::value()
u = u * repeat_u;
v = v * repeat_v;

// Fractional periodic wrap
u = u - std::floor(u);
v = v - std::floor(v);

float x = u * float(imageWidth) - 0.5f;
float y = (1.0f - v) * float(imageHeight) - 0.5f;

int x0 = int(std::floor(x));
int y0 = int(std::floor(y));
int x1 = x0 + 1;
int y1 = y0 + 1;

// Toroidal circular modulo addressing
x0 = (x0 % imageWidth + imageWidth) % imageWidth;
x1 = (x1 % imageWidth + imageWidth) % imageWidth;
y0 = (y0 % imageHeight + imageHeight) % imageHeight;
y1 = (y1 % imageHeight + imageHeight) % imageHeight;
```

This enables seamless tiling across ground planes, walls, and curved primitives.

### f. Deliverables

| Labeled Quadrant Grid (`8_primitive_griglia.bmp`) | Custom Textures with Repeat Wrap (`8_primitive_scelta.bmp`) |
|:---:|:---:|
| ![Labeled Quadrant Grid](./deliverables/8_primitive_griglia.bmp) | ![Custom Textures](./deliverables/8_primitive_scelta.bmp) |
| Grid applied to sphere, capped cylinder, and box | Earth sphere, cat cylinder (repeat $u=2$), checkerboard box ($2\times 2$) |

---

## 6. Motion Blur & Moving Instances

A camera shutter remains open across a finite temporal interval $[t_0, t_1]$. Objects moving during this exposure leave a characteristic motion streak (motion blur).

### a. Temporal Ray Parameter ($\tau$)

Every ray carries an instantaneous emission time $\tau \in [t_0, t_1]$:

$$
\tau = t_0 + \xi (t_1 - t_0), \quad \xi \sim \mathcal{U}(0, 1)
$$

In multithreaded rendering, primary rays sample $N$ stratified shutter instants across $[t_0, t_1]$:

$$
\xi_k = \frac{k}{N}, \quad k \in \{0, 1, \dots, N-1\}
$$

```C++
// ray.h
class ray {
public:
    point3 o;
    vec3 d;
    float tau = 0.0f; // time instant
    ...
    float time() const { return tau; }
};
```

#### Shadow Ray Temporal Continuity
When shading an intersection point $\mathbf{p}$ at time $\tau$, the generated shadow ray **must inherit identical time**:

```C++
// camera.h - ray_color()
ray shadow_ray(rec.p, l, r.time());
```

If the shadow ray loses $\tau$ (defaulting to static $\tau = 0$), cast shadows remain frozen in place while the visible object moves, causing detached and physically inconsistent shadow silhouettes.

### b. Moving Instances vs. Mesh Geometry Rebuild

For analytic spheres, motion can be evaluated by interpolating the center $\mathbf{C}(\tau) = (1-s)\mathbf{C}_0 + s\mathbf{C}_1$. However, this does not generalize to complex geometry:

- A triangle mesh with 100,000 vertices cannot recompute vertex positions and rebuild its BVH acceleration structure for every ray without destroying performance.
- In our engine, **TinyBVH builds once in object space at load time**.
- The underlying primitive geometry remains **completely static**.
- All temporal dynamics are encapsulated by a time-varying affine transformation instance (`moving_instance`).

```
World Ray (time tau) ──[ M⁻¹(tau) ]──> Object Space (Static BVH / Primitive) ──[ M(tau) ]──> World Hit
```

### c. The Golden Rule: Interpolate Parameters, Never Matrices

A fundamental rule of computer graphics emphasized in the course specification:

> *For translations interpolate the vector; for rotations interpolate the angle; never interpolate the matrix elements.*

If two transformation matrices $\mathbf{M}_0$ and $\mathbf{M}_1$ are blended linearly as $(1-s)\mathbf{M}_0 + s\mathbf{M}_1$:
1. Orthogonality is lost ($\mathbf{M}(s)^T \mathbf{M}(s) \ne \mathbf{I}$).
2. For an $80^\circ$ or $180^\circ$ rotation, the intermediate determinant shrinks towards zero ($\det \to 0$), causing severe shearing, warping, and artificial collapse.

Instead, we decompose motion into physical degrees of freedom over normalized shutter progress $s = \text{clamp}((\tau - t_0) / (t_1 - t_0), 0, 1)$:

$$
\mathbf{T}(s) = (1-s)\mathbf{T}_0 + s\mathbf{T}_1, \quad \theta(s) = (1-s)\theta_0 + s\theta_1
$$

The instantaneous matrices $\mathbf{M}(\tau)$ and $\mathbf{M}^{-1}(\tau)$ are synthesized dynamically for each ray.

### d. Thread-Safe Implementation (`moving_instance`)

Under multithreaded rendering (`parallel_for`), rays with different timestamps $\tau$ intersect scene objects concurrently. If an instance modifies its internal matrix member variables, threads clobber each other's state, causing severe data races and visual corruption.

To guarantee zero data races without locks, `moving_instance` constructs a lightweight thread-local `instance` on the execution stack:

```C++
// instance.h
class moving_instance : public instance {
public:
    float t0 = 0.0f, t1 = 0.0f;
    vec3 trans0, trans1;
    float rot_y0 = 0.0f, rot_y1 = 0.0f;

    moving_instance(hittable* obj, material* m, float _t0, float _t1,
                    vec3 _trans0, vec3 _trans1, float _rot_y0 = 0.0f, float _rot_y1 = 0.0f)
        : instance(obj, m), t0(_t0), t1(_t1),
          trans0(_trans0), trans1(_trans1),
          rot_y0(_rot_y0), rot_y1(_rot_y1) {}

    instance get_instance_at(float tau) const {
        float s = (t1 > t0) ? std::clamp((tau - t0) / (t1 - t0), 0.0f, 1.0f) : 0.0f;
        vec3 cur_trans = (1.0f - s) * trans0 + s * trans1;
        float cur_rot_y = (1.0f - s) * rot_y0 + s * rot_y1;

        instance inst(hittable_ptr, mat);
        inst.identity();
        inst.translate(cur_trans);
        inst.rotate_y(cur_rot_y);
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
```

### e. Benchmark Scenes (Slide 15 Deliverables)

We implemented the two required diagnostic setups in `scenes.h`:

1. **Bouncing Scene (`create_motion_blur_bouncing_scene`)**:
   - Terracotta sphere bouncing vertically from $y = 4.0$ to $y = 1.5$.
   - Blue sphere static on the checkerboard ground plane ($y = 1.5$).
   - Directional key light casting soft moving shadows.
   - Deliverables:
     - `ferma` ($t_0 = t_1 = 0$, $32\text{ spp}$): completely sharp baseline.
     - `rimbalzo, 48 istanti` ($t_0 = 0, t_1 = 1$, $48\text{ spp}$): smooth vertical blur on the bouncing sphere and shadow, while the adjacent blue sphere remains razor-sharp.

| Static Baseline (`5_motion_blur_rimbalzo_fermo.bmp`) | Vertical Bouncing Motion Blur (`5_motion_blur_rimbalzo_mosso.bmp`) |
|:---:|:---:|
| ![Bouncing Static](./deliverables/5_motion_blur_rimbalzo_fermo.bmp) | ![Bouncing Motion Blur](./deliverables/5_motion_blur_rimbalzo_mosso.bmp) |
| Static spheres ($t_0 = t_1 = 0$, 32 spp) | Terracotta sphere in harmonic bounce (48 spp) |

2. **Rotating Scene (`create_motion_blur_rotating_scene`)**:
   - Spherical globe resting on the checkerboard plane.
   - Rotates $\theta_0 = 0^\circ \to \theta_1 = 80^\circ$ around its vertical axis during exposure.
   - Motion blur follows the parallels (lines of latitude), while polar vertices remain stationary, exactly demonstrating axial rotation blur.

| Static Baseline (`5_motion_blur_rotazione_fermo.bmp`) | Axial Rotation Motion Blur (`5_motion_blur_rotazione_mosso.bmp`) |
|:---:|:---:|
| ![Rotating Static](./deliverables/5_motion_blur_rotazione_fermo.bmp) | ![Rotating Motion Blur](./deliverables/5_motion_blur_rotazione_mosso.bmp) |
| Static Earth globe ($t_0 = t_1 = 0$, 32 spp) | Earth globe rotating $80^\circ$ around polar axis (48 spp) |

---

## 7. Architectural Infrastructure & Engineering Achievements

### a. RAII Scene Lifecycle Management (`SceneData`)

To guarantee zero memory leaks and clean state resets when switching between scenes, we implemented the `SceneData` container in `scenes.h`:

```C++
struct SceneData {
    hittable_list world;
    light_list lights;
    camera cam;

    std::list<mesh*> meshes;
    std::list<texture*> textures;
    std::list<material*> materials;
    std::list<light*> allocated_lights;
    std::list<ext_light*> allocated_ext_lights;

    void cleanup() {
        for (auto m : meshes) delete m;
        for (auto t : textures) delete t;
        for (auto mat : materials) delete mat;
        for (auto l : allocated_lights) delete l;
        for (auto el : allocated_ext_lights) delete el;

        meshes.clear(); textures.clear(); materials.clear();
        allocated_lights.clear(); allocated_ext_lights.clear();
        world.clear(); lights.clear();
    }
};
```

All heap allocations created during scene construction are tracked and deallocated by `cleanup_scene()`, preventing pointer accumulation across multiple rendering passes.

### b. Spotlight Unit Test Suite

To guarantee mathematical correctness and guard against regressions, we created an independent test suite in `II/src/tests/` (executable via `tests.vcxproj` or CMake target `tests`):

```C++
// tests/spotlight_tests.cpp
int run_spotlight_tests(const char* filter) {
    int failed = 0;
    failed += run("test_initialization", test_initialization, filter);
    failed += run("test_smoothness_clamping", test_smoothness_clamping, filter);
    failed += run("test_direction_normalization", test_direction_normalization, filter);
    failed += run("test_on_axis_peak_intensity", test_on_axis_peak_intensity, filter);
    failed += run("test_inner_cone_linear_decay", test_inner_cone_linear_decay, filter);
    failed += run("test_inner_cone_monotonic_decrease", test_inner_cone_monotonic_decrease, filter);
    failed += run("test_outer_cone_exponential_decay", test_outer_cone_exponential_decay, filter);
    failed += run("test_outer_cone_boundary_cutoff", test_outer_cone_boundary_cutoff, filter);
    failed += run("test_sharp_boundary_when_smoothness_one", test_sharp_boundary_when_smoothness_one, filter);
    failed += run("test_zero_smoothness_immediate_outer_cone", test_zero_smoothness_immediate_outer_cone, filter);
    failed += run("test_distance_invariance", test_distance_invariance, filter);
    failed += run("test_radial_symmetry", test_radial_symmetry, filter);
    failed += run("test_perpendicular_and_behind", test_perpendicular_and_behind, filter);
    failed += run("test_arbitrary_position_and_direction", test_arbitrary_position_and_direction, filter);
    return failed;
}
```

The 14 unit tests verify:
- Boundary conditions and clamping for $\text{smoothness} \in [0, 1]$.
- Peak unit intensity along the central cone axis.
- Monotonic falloff within the inner cone.
- Quadratic decay across the penumbra zone.
- Exact zero-cutoff outside $\theta_0$ and behind the light source.
- Complete radial symmetry under arbitrary rotations around the cone axis.
- Independence of angular attenuation from ray distance.

### c. Multithreaded Parallel Rendering

Scanline rendering is parallelized using Microsoft Parallel Patterns Library (`concurrency::parallel_for`) with OpenMP compatibility shims for Linux.

Instead of writing directly to the window surface during worker thread execution (which causes race conditions and thread collisions in SDL), threads write to a linear color matrix buffer (`std::vector<color> matrix`). The main thread presents the completed buffer via `SDL_RenderPresent`, guaranteeing thread safety and smooth screen updates.

### d. Interactive Controls & Screenshots

The SDL event loop handles real-time user input:
- `S`: Saves an uncompressed 24-bit BMP screenshot (`screenshot.bmp`) directly from the renderer buffer.
- `ESC` / Window Close: Gracefully terminates the application and triggers RAII memory cleanup.

---

## 8. Animated Short Film (Homework 10)

The final homework combines the engine features into a scripted animated short film.

### a. Overview
- **Multi-Scene Pipeline**: Decoupled scene builder architecture consuming a queue of scene-factory lambda functions.
- **11 Scripted Sequences**: Total of 11 sequential scenes composing the short narrative.
- **Environment Lighting**: Equirectangular background skies with automated solar direction alignment and complementary fill lights to preserve dynamic range and shadow detail.
- **Dynamic Motion**: Coordinated camera splines and moving instances evaluated with temporal motion blur.

### b. Deliverables

| Animated Short (`short.gif`) |
|:---:|
| ![Animated Short](./deliverables/short.gif) |

| Keyframe Highlight (`best_frame.bmp`) |
|:---:|
| ![Best Frame](./deliverables/best_frame.bmp) |

Full high-resolution frame sequences and final video footage are accessible on [Google Drive](https://drive.google.com/drive/folders/1Jych7xDYYqV_k2B3mLIN_ZAGH2liG-JR?usp=sharing). External asset attributions are detailed in `LICENSE.txt`.

