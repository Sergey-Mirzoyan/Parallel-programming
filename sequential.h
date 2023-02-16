#ifndef PPP_SEQUENTIAL_H
#define PPP_SEQUENTIAL_H

#include <vector>
#include "preform.h"

namespace sequential
{
    void fill(std::vector<Preform> &res, const std::vector<int> &requirements, int preform_size);
}

#endif //PPP_SEQUENTIAL_H
