#ifndef LIGHT_LIST_H
#define LIGHT_LIST_H

#include "light.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class light_list {
public:
  std::vector<light*> sources;

  light_list() {}
  light_list(light* source) { add(source); }
  light_list(light& source) { add(&source); }

  void clear() { sources.clear(); }

  void add(light* source) {
    sources.push_back(source);
  }
};

#endif
