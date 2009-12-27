#include <vector>

#include <omptl/omptl_numeric>
#include <omptl/omptl_algorithm>

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <omp.h>

const unsigned N = 100 * (1 << 20);

template <typename T>
struct Sqrt
{
	T operator()(const T &x) const { return std::sqrt(x); }
};

int main (int argc, char * const argv[])
{
	// Number of threads is derived from environment
	// variable "OMP_NUM_THREADS"
	std::cout << "Threads: " << omp_get_max_threads() << std::endl;

	std::vector<int> v1(N);

	omptl::generate(v1.begin(), v1.end(), std::rand);
	omptl::sort(v1.begin(), v1.end());
	omptl::random_shuffle(v1.begin(), v1.end());

	std::vector<int> v2(N);
	omptl::transform(v1.begin(), v1.end(), v2.begin(), Sqrt<int>());
	std::cout << "Nr 3's: " << omptl::count(v2.begin(), v2.end(), 3)
		<< std::endl;
	std::cout << "Sum: "
		<< omptl::accumulate(v2.begin(), v2.end(), 0) << std::endl;

	std::cout << *v1.begin() << std::endl;

	return 0;
}

