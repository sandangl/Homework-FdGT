#ifndef INTERVAL_H
#define INTERVAL_H

#include "common.h"

class interval {
public:
  float min, max;

  interval() : min(+infinity), max(-infinity) {} // Default interval is empty

  interval(float _min, float _max) : min(_min), max(_max) {}

  bool contains(float x) const {
    return min <= x && x <= max;
  }

  bool surrounds(float x) const {
    return min < x && x < max;
  }

  float clamp(float x) const {
    if (x < min) return min;
    if (x > max) return max;
    return x;
  }

  static const interval empty, universe;
};

// Definizione dei membri statici dichiarati nella classe (interval::empty, interval::universe).
const interval interval::empty = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);

#endif
