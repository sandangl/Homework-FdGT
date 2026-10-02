#ifndef POINT3_LIST_H
#define POINT3_LIST_H

#include "common.h"

#include <memory>
#include <vector>

class point3_list : public point3 {
public:
  std::vector<point3> points;

  point3_list() {}
  point3_list(point3 point) { add(point); }

  void clear() { points.clear(); }

  void add(point3 point) {
    points.push_back(point);
  }

};

using path = point3_list;

#endif
