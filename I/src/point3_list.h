#ifndef POINT3_LIST_H
#define POINT3_LIST_H

#include "common.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class point3_list : public point3 {
public:
  std::vector<shared_ptr<point3>> points;

  point3_list() {}
  point3_list(shared_ptr<point3> point) { add(point); }

  void clear() { points.clear(); }

  void add(shared_ptr<point3> point) {
    points.push_back(points);
  }

};

using path = point3_list;

#endif
