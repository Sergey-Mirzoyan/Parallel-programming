#ifndef PPP_PARALLEL_H
#define PPP_PARALLEL_H

#include <vector>
#include "preform.h"

namespace parallel
{
    void fill(std::vector<Preform> &res, const std::vector<int> &requirements, int preform_size, int preprocessing_amount);
}

#endif //PPP_PARALLEL_H
