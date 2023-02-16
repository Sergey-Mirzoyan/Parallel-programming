#ifndef PPP_PREFORM_H
#define PPP_PREFORM_H

#include <vector>

class Preform
{
public:
    Preform(unsigned size = 0);

    bool cut(unsigned len);

    bool undo(unsigned len);

    [[nodiscard]] inline unsigned used() const { return _used; }

    inline const std::vector<unsigned> &get_cuts() { return cuts; }

    [[nodiscard]] inline unsigned serialized_length() const { return sizeof(unsigned) * (3 + cuts.size()); }

    void serialize(char *ptr) const;
    void deserialize(char *data);

private:
    unsigned size;
    unsigned _used;
    std::vector<unsigned> cuts;
};


#endif //PPP_PREFORM_H
