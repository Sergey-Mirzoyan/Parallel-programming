#include <algorithm>
#include <cstring>
#include "preform.h"

Preform::Preform(unsigned size) : size(size), _used(0) {}

bool Preform::cut(unsigned len)
{
    if ((len + _used) > size)
        return false;
    _used += len;
    cuts.push_back(len);
    return true;
}

bool Preform::undo(unsigned len)
{
    auto pos = std::find(cuts.begin(),cuts.end(), len);
    if (pos != cuts.end())
    {
        _used -= len;
        cuts.erase(pos);
        return true;
    }
    return false;
}

void Preform::serialize(char *dst) const
{
    char *ptr = dst;
    unsigned amount = cuts.size();
    memcpy(ptr, &size, sizeof(unsigned));
    ptr += sizeof(unsigned);
    memcpy(ptr, &_used, sizeof(unsigned));
    ptr += sizeof(unsigned);
    memcpy(ptr, &amount, sizeof(unsigned));
    ptr += sizeof(unsigned);
    memcpy(ptr, cuts.data(), sizeof(unsigned) * amount);
}

void Preform::deserialize(char *data)
{
    unsigned amount;
    memcpy(&size, data, sizeof(unsigned));
    data += sizeof(unsigned);
    memcpy(&_used, data, sizeof(unsigned));
    data += sizeof(unsigned);
    memcpy(&amount, data, sizeof(unsigned));
    data += sizeof(unsigned);
    cuts.assign((unsigned*)data, (unsigned*)data + amount);
}
