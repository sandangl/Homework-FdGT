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
    // Each channel gets multiplied by a constant that mimicks human perception of red, blue and green.
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

TODO: fill with juicy docs
