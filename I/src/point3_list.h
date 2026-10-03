#ifndef POINT3_LIST_H
#define POINT3_LIST_H

#include "common.h"

#include <memory>
#include <vector>

class point3_list {
public:
  std::vector<point3> points;

  point3_list() {}
  point3_list(point3 point) { add(point); }

  int length() const { return static_cast<int>(points.size()); }

  void clear() { points.clear(); }

  void add(point3 point) {
    points.push_back(point);
  }

  void add(point3_list l) {
    points.insert(points.end(), l.points.begin(), l.points.end());
  }
};

using path = point3_list;

#endif
