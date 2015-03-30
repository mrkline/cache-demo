#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <limits>

#include <cassert>
#include <cstdint> // For standard int types
#include <cstring> // For memcpy

using namespace std;
using namespace std::chrono;

// Some constants we will use:

// The cache size of your processor, in bytes. Adjust accordingly.
const size_t cacheSize = 8 * 1024 * 1024; // 8 * kB * mB

// The number of integers that fit in the CPU cache,
// which is useful for picking a sample set size.
const size_t intsInCache = cacheSize / sizeof(int);

// Used to avoid typing "high_resolution_clock" repeatedly
// Note that the HRC is not guaranteed to be monotonic,
// but this shouldn't be a problem so long as you don't
// change your OS clock or go through a Daylight Savings Time change
// while running this. ಠ_ಠ
using clk = high_resolution_clock;

// Invalidates the entire CPU cache so that it has minimal impact on
// our timings.
void clearCache()
{
	// A dumb but effective way to clear the cache is to copy
	// as much memory as there is cache.
	static uint8_t buffA[cacheSize];
	static uint8_t buffB[cacheSize];
	memcpy(buffA, buffB, cacheSize);
}

// Populates each integer in the given data set
// using the given random number generator
template<typename R>
void populateDataSet(vector<int>& data, R rng)
{
	for (int& d : data)
		d = rng();

}

int doWork(const vector<int>& data)
{
	unsigned long sum = 0;

	// Square each value
	for (int d : data)
		sum += d * d;

	return (int)(sum / data.size());
}

int main()
{
	const unsigned int iterations = 1000; // Number of tests to run

	// Gather the program start time so we can tell how long it ran total.
	const auto programStartTime = clk::now();

	// Our test data set
	auto data = vector<int>(intsInCache * 10);

	// Used for populating our data set each time before we run
	random_device rd;
	// Seed the RNG with actual hardware/OS randomness from random_device
	default_random_engine re(rd());
	// Since the "work" we are doing is squaring each integer,
	// initialize them with some value between 0 and the square root of the integer max
	uniform_int_distribution<int> ud(1, 10);
	// Bundle this all up into a closure that populateDataSet can call:
	auto rng = [&] { return ud(re); };

	// Used to sum how much time all of our runs took,
	// excluding the setup and measurement work we do around them.
	clk::duration runSum(0);

	for (unsigned int i = 0; i < iterations; ++i) {
		populateDataSet(data, rng);
		clearCache();

		// ...and go!
		const auto runStart = clk::now();
		const int result = doWork(data);
		const auto runTime = clk::now() - runStart;
		runSum += runTime;
		// We write out the result to make sure the compiler doesn't
		// eliminate the work as a dead store,
		// and to give us something to look at.
		cout << "Run " << i + 1 << ": " << result << "\r";
		cout.flush();
	}
	cout << "\n";

	const auto cumulativeTime = duration_cast<milliseconds>(runSum).count();
	const auto averageRunTime = duration_cast<microseconds>(runSum).count() / iterations;
	const auto actualRuntime = duration_cast<milliseconds>(clk::now() - programStartTime).count();

	cout << "Ran for a total of " << actualRuntime / 1000 << "." << actualRuntime % 1000
	     << " seconds (including bookkeeping and cache clearing)\n";
	cout << iterations << " runs took " << cumulativeTime / 1000 << "." << cumulativeTime % 1000
	     << " seconds total,\n";
	cout << "Averaging " << averageRunTime / 1000 << "." << averageRunTime % 1000
	     << " milliseconds per run\n";
	return 0;
}
