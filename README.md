# Homework FdGT

This repo is a journal for the 3D Graphics Fundamentals course (University of Basilicata) for the 2026/2027 academic year.

---

## First Homework

In this homework (refer to [I/](./I/)), we experimented with camera improvements, introducing:

- **3D Anaglyph rendering**: executed via two sequential render passes in the viewport for a left (red) and right (cyan) camera. Tweaking the interocular distance value and sign controls the intensity and inversion of the 3D effect.
- **Camera Animation**: interpolated along a Catmull-Rom spline from specified camera positions using high-performance vectorized calculations via custom vector and matrix utility classes. Each frame can be saved and assembled into video footage with FFmpeg to produce 10 seconds of footage at 25 fps.
- **Control Improvements**: implemented non-blocking animation rendering within the main event loop, allowing the main thread to listen for keyboard events. Added *camera controls* for near real-time movement, *anaglyph toggling* to enable/disable the 3D effect, *frame recording toggles*, and *animation toggling*.

[Full documentation](./I/README.md)

<div style="display:flex; flex-direction: row; gap:16px; padding-top:8px;">
    <img src="./assets/animationCatmullRom@25fps.gif" width="50%"/>
    <img src="./assets/animationCatmulRomNoAn@25fps.gif" width="50%"/>
</div>