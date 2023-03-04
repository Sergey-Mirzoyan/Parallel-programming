#include <iostream>
#include <chrono>
#include <mpi.h>
#include "parallel.h"
#include "sequential.h"
//#define  CPP_TIME


void time_results(int req_len, int preform_size, int preprocessing_amount, int repeats)
{
    #ifdef CPP_TIME
    #define timer() std::chrono::steady_clock::now()
    #define count(s, e) std::chrono::duration_cast<std::chrono::nanoseconds>((e) - (s)).count()
    #define avg(s, r) (double(s) / r) * 1e-9
    unsigned long long sum1 = 0, sum2 = 0;
    #else
    #define timer() MPI_Wtime()
    #define count(s, e) ((e) - (s))
    #define avg(s, r) ((s) / (r))
    double sum1 = 0, sum2 = 0;
    #endif

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//    srandom(time(nullptr));

    std::vector<Preform> res1, res2;
    std::vector<int> requirements;

    if (!rank)
        std::cout << "\nRepeating measures " << repeats << " times\n";


    for (int i = 0; i < repeats; ++i)
    {
        requirements.clear();
        for (int j = 0; j < req_len; ++j)
            requirements.push_back(int(random() % preform_size) + 1);

        auto start_p = timer();
        parallel::fill(res2, requirements, preform_size, preprocessing_amount);
        auto end_p = timer();
        sum2 += count(start_p, end_p);


        if (!rank)
        {
            std::cout << "Parallel " << i << " completed\n";

            auto start_s = timer();
            sequential::fill(res1, requirements, preform_size);
            auto end_s = timer();
            sum1 += count(start_s, end_s);

            std::cout << "Sequential " << i << " completed\n";
        }

    }
    if (!rank)
    {
        std::cout << "\nSequential algorithm calculated result in average of " << std::fixed
                  << avg(sum1, repeats) << " seconds\n";
        std::cout << "\nParallel algorithm calculated result in average of " << std::fixed
                  << avg(sum2, repeats) << " seconds\n\n";

        std::cout << "\nLast sequential result:\n\n";
        for (int i = 0; i < res1.size(); ++i)
        {
            std::cout << "Preform " << i << ": " << preform_size - res1[i].used() << " unused. Cuts: ";
            for (unsigned cut: res1[i].get_cuts())
                std::cout << cut << ' ';
            std::cout << std::endl;
        }

        std::cout << "\nLast parallel result:\n\n";
        for (int i = 0; i < res2.size(); ++i)
        {
            std::cout << "Preform " << i << ": " << preform_size - res2[i].used() << " unused. Cuts: ";
            for (unsigned cut: res2[i].get_cuts())
                std::cout << cut << ' ';
            std::cout << std::endl;
        }
    }
}

int main()
{
    MPI_Init(nullptr, nullptr);
    time_results(3000, 100, 1, 1);
    MPI_Finalize();
    return 0;
}
