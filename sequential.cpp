#include "sequential.h"

namespace sequential
{
    void fill_rec(std::vector<Preform> &res, std::vector<Preform> preforms, std::vector<int> slots,
                  const std::vector<int> &requirements, unsigned start_req)
    {
        int preform_num;
        bool found = true;
        for (unsigned i = start_req; found && i < requirements.size(); ++i)
        {
            found = false;
            for (preform_num = 0; preform_num < preforms.size() && !found; ++preform_num)
            {
                if (preforms[preform_num].cut(requirements[i]))
                {
                    found = true;
                    slots[i] = preform_num;
                }
            }
        }
        if (found)
        {
            int last_used = static_cast<int>(preforms.size() - 1);
            while (last_used >= 0 && !preforms[last_used].used())
                --last_used;
            if (last_used + 1 < res.size())
            {
                preforms.resize(last_used + 1);
                res = std::vector<Preform>(preforms);


                for (unsigned i = requirements.size() - 2; i > start_req; --i)
                {
                    int next = slots[i] + 1;

                    preforms[slots[i]].undo(requirements[i]);
                    if (slots[i + 1] >= 0)
                        preforms[slots[i + 1]].undo(requirements[i + 1]);
                    slots[i] = slots[i + 1] = -1;

                    for (preform_num = next, found = false; preform_num < preforms.size() && !found; ++preform_num)
                    {
                        if (preforms[preform_num].cut(requirements[i]))
                        {
                            found = true;
                            slots[i] = preform_num;
                        }
                    }
                    fill_rec(res, preforms, slots, requirements, i + 1);
                }
            }
        }
    }

    void fill(std::vector<Preform> &res, const std::vector<int> &requirements, int preform_size)
    {
        std::vector<Preform> preforms(requirements.size(), Preform(preform_size));
        std::vector<int> slots(requirements.size(), -1);

        res = std::vector<Preform>(preforms);
        for (int i = 0; i < requirements.size(); ++i)
            res[i].cut(requirements[i]);
            int x;
            x = 2*2*2;

        fill_rec(res, preforms, slots, requirements, 0);
    }
}
