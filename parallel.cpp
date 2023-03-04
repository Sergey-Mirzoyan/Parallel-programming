#include <mpi.h>
#include <omp.h>
#include "parallel.h"


namespace parallel
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

    typedef std::vector<std::pair<std::vector<Preform>, std::vector<int>>> Pool;

    void fill_firstn(Pool &pool, std::vector<Preform> preforms, std::vector<int> slots,
                     const std::vector<int> &requirements, unsigned start_req, unsigned max_size)
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
            if (last_used + 1 < max_size)
            {
                std::vector<Preform> option(preforms);
                option.resize(last_used + 1);
                #pragma omp critical (pool)
                pool.emplace_back(option, slots);
                for (int i = int(requirements.size()) - 1; i >= int(start_req); --i)
                {
                    int next = slots[i] + 1;

                    preforms[slots[i]].undo(requirements[i]);
                    slots[i] = -1;

                    for (preform_num = next; preform_num < preforms.size(); ++preform_num)
                    {
                        if (preforms[preform_num].cut(requirements[i]))
                        {
                            slots[i] = preform_num;
                            fill_firstn(pool, preforms, slots, requirements, i + 1, max_size);
                            preforms[slots[i]].undo(requirements[i]);
                            slots[i] = -1;
                        }
                    }
                }
            }
        }
    }

    int serialize(const std::vector<Preform> &preforms, char *&data)
    {
        int len = 0;
        for (const Preform &p: preforms)
            len += static_cast<int>(p.serialized_length());
        data = new char[len];
        char *ptr = data;
        for (const Preform &p: preforms)
        {
            p.serialize(ptr);
            ptr += p.serialized_length();
        }
        return len;
    }

    void deserialize(std::vector<Preform> &preforms, char *data, int len)
    {
        char *end = data + len;
        int i = 0;
        while (data < end)
        {
            preforms[i].deserialize(data);
            data += preforms[i].serialized_length();
            ++i;
        }
    }

    enum
    {
        TAG_REQUEST = 1,
        TAG_RESULT,
        TAG_DATA,
        TAG_FLAG,
        TAG_DONE
    };

    void fill(std::vector<Preform> &res, const std::vector<int> &requirements, int preform_size,
              const int preprocessing_amount)
    {
        int rank;

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        if (!rank)
        {
            Pool pool;
            bool in_process = true;
            #pragma omp parallel num_threads(2) default(none) shared(requirements, res, preform_size, pool, in_process, ompi_mpi_int, ompi_mpi_char, ompi_mpi_unsigned, ompi_mpi_cxx_bool, ompi_mpi_comm_world)
            {
                unsigned max_size = requirements.size();
                #pragma omp master
                {
                    char buf;
                    MPI_Status status;
                    int comm_size;
                    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
                    int active_children = comm_size - 1;
                    res = std::vector<Preform>(requirements.size(), Preform(0));
                    for (int i = 0; i < requirements.size(); ++i)
                        res[i].cut(requirements[i]);
                    while (active_children)
                    {
                        MPI_Recv(&buf, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                        if (status.MPI_TAG == TAG_REQUEST)
                        {
                            bool go;
                            #pragma omp critical (in_process)
                            if (in_process)
                            {
                                do
                                {
                                    #pragma omp critical (pool)
                                    go = pool.empty();
                                } while (go);
                            }
                            #pragma omp critical (pool)
                            go = !pool.empty();
                            if (go)
                            {
                                MPI_Send(&go, 1, MPI_CXX_BOOL, status.MPI_SOURCE, TAG_FLAG, MPI_COMM_WORLD);
                                MPI_Send(&max_size, 1, MPI_UNSIGNED, status.MPI_SOURCE, TAG_DATA, MPI_COMM_WORLD);
                                char *data;
                                int len;
                                #pragma omp critical(pool)
                                len = serialize(pool[0].first, data);
                                MPI_Send(&len, 1, MPI_INT, status.MPI_SOURCE, TAG_DATA, MPI_COMM_WORLD);
                                MPI_Send(data, len, MPI_CHAR, status.MPI_SOURCE,
                                         TAG_DATA, MPI_COMM_WORLD);
                                delete[]data;
                                #pragma omp critical(pool)
                                MPI_Send(pool[0].second.data(), int(preprocessing_amount), MPI_INT, status.MPI_SOURCE,
                                         TAG_DATA, MPI_COMM_WORLD);
                                #pragma omp critical(pool)
                                pool.erase(pool.begin());
                            } else
                            {
                                bool flag = false;
                                MPI_Send(&flag, 1, MPI_CXX_BOOL, status.MPI_SOURCE, TAG_FLAG, MPI_COMM_WORLD);
                            }
                        } else if (status.MPI_TAG == TAG_RESULT)
                        {
                            unsigned size;
                            MPI_Recv(&size, 1, MPI_UNSIGNED, status.MPI_SOURCE, TAG_DATA, MPI_COMM_WORLD, &status);
                            if (size < max_size)
                            {
                                bool interested = true;
                                MPI_Send(&interested, 1, MPI_CXX_BOOL, status.MPI_SOURCE, TAG_FLAG, MPI_COMM_WORLD);
                                std::vector<Preform> local_res(size, Preform(0));
                                int len;
                                MPI_Recv(&len, 1, MPI_INT, status.MPI_SOURCE, TAG_DATA, MPI_COMM_WORLD, &status);
                                char *data = new char[len];
                                MPI_Recv(data, len, MPI_CHAR, status.MPI_SOURCE, TAG_DATA,
                                         MPI_COMM_WORLD, &status);
                                deserialize(local_res, data, len);
                                delete[]data;
                                res = local_res;
                                max_size = size;
                            } else
                            {
                                bool interested = false;
                                MPI_Send(&interested, 1, MPI_CXX_BOOL, status.MPI_SOURCE, TAG_FLAG, MPI_COMM_WORLD);
                            }
                        } else if (status.MPI_TAG == TAG_DONE)
                            --active_children;
                    }
                }
                #pragma omp single
                {
                    std::vector<int> req(preprocessing_amount);
                    std::copy(requirements.begin(), requirements.begin() + preprocessing_amount, req.begin());
                    std::vector<Preform> preforms(req.size(), Preform(preform_size));
                    std::vector<int> slots(req.size(), -1);

                    fill_firstn(pool, preforms, slots, req, 0, preprocessing_amount + 1);

                    #pragma omp critical (in_process)
                    in_process = false;
                }
            }
        } else
        {
            char buf = 0;
            #pragma omp parallel num_threads(4) default(none) shared(rank, buf, requirements, preform_size, ompi_mpi_int, ompi_mpi_char, ompi_mpi_unsigned, ompi_mpi_cxx_bool, ompi_mpi_comm_world)
            {
                bool go = true;
                do
                {
                    MPI_Status status;
                    unsigned max_size;
                    std::vector<Preform> preforms;
                    std::vector<int> slots;
                    #pragma omp critical (communication)
                    {
                        MPI_Send(&buf, 1, MPI_CHAR, 0, TAG_REQUEST, MPI_COMM_WORLD);
                        MPI_Recv(&go, 1, MPI_CXX_BOOL, 0, TAG_FLAG, MPI_COMM_WORLD, &status);
                        if (go)
                        {
                            int len;
                            MPI_Recv(&max_size, 1, MPI_UNSIGNED, 0, TAG_DATA, MPI_COMM_WORLD, &status);

                            preforms = std::vector<Preform>(max_size, Preform(preform_size));
                            slots = std::vector<int>(requirements.size(), -1);
                            MPI_Recv(&len, 1, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD, &status);
                            char *data = new char[len];
                            MPI_Recv(data, len, MPI_CHAR, 0, TAG_DATA,
                                     MPI_COMM_WORLD, &status);
                            deserialize(preforms, data, len);
                            delete[]data;
                            MPI_Recv(&slots.front(), preprocessing_amount, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD,
                                     &status);

                        }
                    }
                    if (go)
                    {
                        std::vector<Preform> local_res = std::vector<Preform>(max_size, Preform(preform_size));

                        fill_rec(local_res, preforms, slots, requirements, preprocessing_amount);

                        max_size = local_res.size();
                        #pragma omp critical (communication)
                        {
                            MPI_Send(&buf, 1, MPI_CHAR, 0, TAG_RESULT, MPI_COMM_WORLD);
                            MPI_Send(&max_size, 1, MPI_UNSIGNED, 0, TAG_DATA, MPI_COMM_WORLD);

                            bool interest;
                            MPI_Recv(&interest, 1, MPI_CXX_BOOL, 0, TAG_FLAG, MPI_COMM_WORLD, &status);
                            if (interest)
                            {
                                char *data;
                                int len = serialize(local_res, data);
                                MPI_Send(&len, 1, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD);
                                MPI_Send(data, len, MPI_CHAR, 0, TAG_DATA, MPI_COMM_WORLD);
                                std::vector<char> debug;
                                debug.assign(data, data + len);
                                delete[]data;
                            }
                        }
                    }
                } while (go);
            }
            MPI_Send(&buf, 1, MPI_CHAR, 0, TAG_DONE, MPI_COMM_WORLD);
        }
    }
}