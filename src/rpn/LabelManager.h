#ifndef LABEL_MANAGER_H
#define LABEL_MANAGER_H

#include <string>

class LabelManager {
  int counter = 0;

 public:
  std::string NewLabel() {
    return "L" + std::to_string(counter++);
  }
};

#endif
