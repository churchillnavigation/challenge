#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <dlfcn.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>

#include "common_interface.hh"

struct program_options
{
    std::size_t num_points = 10000000;
    std::size_t num_queries = 1000;
    std::size_t query_size = 20;
    std::size_t seed = 12345;
    std::vector<std::string> sopaths;
    bool bare_timings = false;
};

enum class sfparse_res
{
    NO_ERROR = 0,
        NOT_VALID = 1,
        EXTRA_CHARS = 2,
        RANGE_ERROR = 3
};

sfparse_res sfparse_ul(
    const char *const c_str,
    unsigned long &output,
    int base
) {
    char *parse_end = nullptr;
    unsigned long result = std::strtoul(c_str, &parse_end, base);
    if(parse_end == c_str)
    {
        return sfparse_res::NOT_VALID;
    }
    else if(*parse_end != '\0')
    {
        return sfparse_res::EXTRA_CHARS;
    }
    else if(std::numeric_limits<unsigned long>::max() == result
            && ERANGE == errno
    ) {
        return sfparse_res::EXTRA_CHARS;
    } else {
        output = result;
        return sfparse_res::NO_ERROR;
    }
}

program_options parse_args(int argc, char **argv)
{
    program_options ret;

    static option opt_structs[] = {
        {"num-points", required_argument, 0, (int)'p'},
        {"num-queries", required_argument, 0, (int)'q'},
        {"query-size", required_argument, 0, (int)'s'},
        {"seed", required_argument, 0, (int)'r'},
        {"bare-timings", no_argument, 0, (int)'b'},
        {0, 0, 0, 0}
    };

    int sel_code = 0;
    int sel_index = 0;

    while(sel_code != -1)
    {
        sel_code = getopt_long(argc, argv, "bp:q:s:r:", opt_structs, &sel_index);

        switch(sel_code)
        {
        case 'b' : {
            ret.bare_timings = true;
            break;
        }
        case 'p': {
            if(sfparse_res::NO_ERROR != sfparse_ul(optarg, ret.num_points, 0))
            {
                std::cerr
                    << "Error: --num-points requires an integer argument."
                    << std::endl;
                std::exit(1);
            }
            break;
        }
        case 'q': {
            if(sfparse_res::NO_ERROR != sfparse_ul(optarg, ret.num_queries, 0))
            {
                std::cerr
                    << "Error: --num-queries requires an integer argument."
                    << std::endl;
                std::exit(1);
            }
            break;
        }
        case 's': {
            if(sfparse_res::NO_ERROR != sfparse_ul(optarg, ret.query_size, 0))
            {
                std::cerr << "Error: --query-size requires an integer argument."
                          << std::endl;
                std::exit(1);
            }
            break;
        }
        case 'r': {
            if(sfparse_res::NO_ERROR != sfparse_ul(optarg, ret.seed, 0))
            {
                std::cerr << "Error: --seed requires an unsigned integer argument."
                          << std::endl;
                std::exit(1);
            }
        }
        case '?':
            break;
        default:
            // Do nothing.
            break;
        }
    }

    for(; optind < argc; ++optind)
    {
        char resolved [PATH_MAX];

        if(nullptr == realpath(argv[optind], resolved))
        {
            std::cerr
                << "Error: Cannot resolve path for "
                << (std::string(argv[optind])) << std::endl;
            std::exit(1);
        }

        ret.sopaths.push_back(std::string(resolved));
    }

    if(ret.sopaths.size() == 0)
    {
        std::cerr << "Please provide at least one shared library to test."
                  << std::endl;
        std::exit(1);
    }

    return ret;
}

struct test_result
{
    std::string sopath;

    std::vector<std::vector<pack_point>> resulting_points;
    std::chrono::high_resolution_clock::duration init_time;
    std::chrono::high_resolution_clock::duration search_time;
};

test_result test_shared_lib(
    const std::string &sopath,
    const std::vector<pack_point> &test_points,
    const std::vector<pack_rect> &queries,
    const int32_t query_size
) {
    test_result ret;
    ret.sopath = sopath;

    const char *err_msg = nullptr;

    void *sohandle = dlopen(sopath.c_str(), RTLD_LAZY);
    if(nullptr == sohandle)
    {
        err_msg = dlerror();

        std::cerr << "dlopen() failed for " << sopath << std::endl;
        std::cerr << (std::string(err_msg)) << std::endl;;
        std::exit(1);
    }

    auto create_fn = reinterpret_cast<create_fn_t>(dlsym(sohandle, "create"));
    if(nullptr == create_fn)
    {
        std::cerr << "dlsym() failed for \"create\" function in " << sopath
                  << std::endl;

        err_msg = dlerror();
        if(nullptr != err_msg)
        {
            std::cerr << (std::string(err_msg)) << std::endl;
        }
        else
        {
            std::cerr << "The symbol was found, but defined as nullptr."
                      << std::endl;
        }
        dlclose(sohandle);
        std::exit(1);
    }

    auto search_fn = reinterpret_cast<search_fn_t>(dlsym(sohandle, "search"));
    if(nullptr == search_fn)
    {
        std::cerr << "dlsym() failed for \"search\" function in " << sopath
                  << std::endl;

        err_msg = dlerror();
        if(nullptr != err_msg)
        {
            std::cerr << (std::string(err_msg)) << std::endl;
        }
        else
        {
            std::cerr << "The symbol was found, but defined as nullptr."
                      << std::endl;
        }
        dlclose(sohandle);
        std::exit(1);
    }

    auto destroy_fn = reinterpret_cast<destroy_fn_t>(
        dlsym(sohandle, "destroy")
    );
    if(nullptr == destroy_fn)
    {
        std::cerr << "dlsym() failed for \"destroy\" function in " << sopath
                  << std::endl;

        err_msg = dlerror();
        if(nullptr != err_msg)
        {
            std::cerr << (std::string(err_msg)) << std::endl;
        }
        else
        {
            std::cerr << "The symbol was found, but defined as nullptr."
                      << std::endl;
        }
        dlclose(sohandle);
        std::exit(1);
    }

    using std::chrono::high_resolution_clock;

    auto t_start = high_resolution_clock::now();

    SearchContext *sc = create_fn(
        &(*test_points.begin()),
        &(*test_points.end())
    );

    auto t_end = high_resolution_clock::now();

    ret.init_time = t_end - t_start;

    if(nullptr == sc)
    {
        std::cerr << "Call to create in " << sopath << " returned null."
                  << std::endl;

        return ret;
    }


    std::vector<pack_point> dummy_output(query_size);

    t_start = high_resolution_clock::now();

    for(const pack_rect &query : queries)
    {
        search_fn(
            sc,
            query,
            query_size,
            dummy_output.data()
        );
    }

    t_end = high_resolution_clock::now();
    ret.search_time = (t_end - t_start);

    for(const pack_rect &query : queries)
    {
        ret.resulting_points.push_back({});
        ret.resulting_points.back().resize(query_size);

        int32_t num_found = search_fn(
            sc,
            query,
            query_size,
            ret.resulting_points.back().data()
        );

        ret.resulting_points.back().resize(num_found);
    }

    sc = destroy_fn(sc);

    if(nullptr != sc)
    {
        std::cerr << "Call to destroy in " << sopath << "did not return null."
                  << std::endl;
        return ret;
    }

    if(dlclose(sohandle))
    {
        std::exit(1);
    }

    return ret;
}

template<class Dist>
struct point_generator
{
    Dist dist;
    int32_t cur_rank;
    int8_t cur_id;

    point_generator(
        Dist dist_in
    )
        : dist(dist_in),
          cur_rank(0),
          cur_id(0)
    {
    }

    template<class Generator>
    pack_point operator()(Generator &g)
    {
        pack_point ret =  {cur_id, cur_rank, dist(g), dist(g)};
        ++cur_rank;
        if(127 == cur_id)
        {
            cur_id = -127;
        }
        else
        {
            ++cur_id;
        }
        return ret;
    }

};

template<class Dist>
struct pack_rect_generator
{
    Dist dist;

    pack_rect_generator(
        Dist dist_in
    )
        : dist(dist_in)
    {
    }

    template<class Gen>
    pack_rect operator()(Gen &g)
    {
        pack_rect ret = {dist(g), dist(g), dist(g), dist(g)};
        if(ret.hx < ret.lx)
        {
            std::swap(ret.lx, ret.hx);
        }
        if(ret.hy < ret.ly)
        {
            std::swap(ret.ly, ret.hy);
        }
        return ret;
    }
};

// Local version of c++14 mismatch.
template<class ItA, class ItB>
std::pair<ItA, ItB> mismatch_cpp14(
    ItA a_src, ItA a_lim,
    ItB b_src, ItB b_lim
) {
    while(a_src != a_lim && b_src != b_lim && (*a_src == *b_src))
    {
        ++a_src;
        ++b_src;
    }
    return std::make_pair(a_src, b_src);
}

void compare_results(
    const test_result &a,
    const test_result &b,
    const std::vector<pack_rect> &queries
) {
    std::cout << std::endl;
    std::cout << "Comparing " << a.sopath << " and " << b.sopath << "\n";

    std::size_t query_ind = 0;
    for(; query_ind < queries.size(); ++query_ind)
    {
        auto a_src = a.resulting_points[query_ind].cbegin();
        auto a_lim = a.resulting_points[query_ind].cend();
        auto b_src = b.resulting_points[query_ind].cbegin();
        auto b_lim = b.resulting_points[query_ind].cend();

        auto mismatch = mismatch_cpp14(a_src, a_lim, b_src, b_lim);

        if(mismatch.first != a_lim || mismatch.second != b_lim)
        {
            std::cout << "Mismatch on query " << query_ind << ":\n";
            std::cout << "Query is " << queries[query_ind] << "\n";

            if(mismatch.first == a_lim)
            {
                std::cout << a.sopath << " gives eol.\n";
            }
            else
            {
                std::cout << a.sopath << " at position " << (mismatch.first - a_src) << " gives " << *(mismatch.first) << "\n";
            }

            if(mismatch.second == b_lim)
            {
                std::cout << b.sopath << " gives eol.\n";
            }
            else
            {
                std::cout << b.sopath << " at position " << (mismatch.second - b_src) << " gives " << *(mismatch.second)
                          << "\n";
            }

            goto loop_end;
        }
    }
 loop_end:

    if(query_ind == queries.size())
    {
        std::cout << "Match.\n";
    }

    std::cout << std::flush;
}

template<class It, class Func>
void for_all_choose_2(It src, It lim, Func func)
{
    It el1 = src;
    while(el1 != lim)
    {
        It el2 = el1;
        ++el2;
        while(el2 != lim)
        {
            func(*el1, *el2);
            ++el2;
        }
        ++el1;
    }
}

int main(int argc, char **argv)
{
    program_options opts = parse_args(argc, argv);

    if(! opts.bare_timings)
    {
        std::cout << "Points requested: " << opts.num_points << "\n"
                  << "Queries requested: " << opts.num_queries << "\n"
                  << "Query size requested: " << opts.query_size << "\n";
    }

    std::vector<pack_point> test_points(opts.num_points);

    std::uniform_real_distribution<float> flt_dist(-1000, 1000);
    point_generator<decltype(flt_dist)> pdist(flt_dist);
    std::mt19937 gen(opts.seed);
    std::generate(
        test_points.begin(),
        test_points.end(),
        [&](){return pdist(gen);}
    );

    pack_rect_generator<decltype(flt_dist)> qdist(flt_dist);

    std::vector<pack_rect> queries(opts.num_queries);
    std::generate(
        queries.begin(),
        queries.end(),
        [&](){return qdist(gen);}
    );

    std::vector<test_result> all_results;

    for(auto &sopath : opts.sopaths)
    {
        test_result result = test_shared_lib(
            sopath,
            test_points,
            queries,
            opts.query_size
        );

        all_results.push_back(result);
    }

    for_all_choose_2(
        all_results.begin(),
        all_results.end(),
        [&](const test_result &a, const test_result &b){
            compare_results(a, b, queries);
        }
    );

    for(auto &result : all_results)
    {
        if(! opts.bare_timings)
        {
            std::cout << result.sopath << " init: " << result.init_time.count() << std::endl;
            std::cout << result.sopath << " search: " << result.search_time.count() << std::endl;
        }
        else
        {
            std::cout << opts.seed << " " << result.search_time.count() << std::endl;
        }
    }
}
