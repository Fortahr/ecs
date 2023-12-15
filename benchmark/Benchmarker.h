#pragma once

#include <iomanip>
#include <iostream>
#include <numeric>

class Benchmarker
{
private:
	static constexpr size_t column_width = 12;

public:
	static void print_header();

	template <size_t Count = 20, typename Func>
	static void benchmark(std::string_view name, Func func, const size_t& callsPerIteration);
};

inline void Benchmarker::print_header()
{
	std::cout << std::left << std::setw(20) << "Test";

	std::cout << std::right;
	std::cout << ' ' << std::setw(column_width) << "Count";
	std::cout << ' ' << std::setw(column_width) << "Median";
	std::cout << ' ' << std::setw(column_width) << "Mean";
	std::cout << ' ' << std::setw(column_width) << "StdDev";
	std::cout << ' ' << std::setw(column_width) << "CI / 2";
	std::cout << '\n';
}

template <size_t Count, typename Func>
__declspec(noinline) void Benchmarker::benchmark(std::string_view name, Func func, const size_t& callsPerIteration)
{
	std::array<double, Count> deltas;

	std::chrono::steady_clock::time_point start, end;

	for (size_t i = 0; i < Count; ++i)
	{
		start = std::chrono::high_resolution_clock::now();

		func();

		end = std::chrono::high_resolution_clock::now();

		deltas[i] = std::chrono::duration<double, std::nano>(end - start).count();
	}

	std::sort(deltas.begin(), deltas.end());

	double median = deltas[Count / 2];
	if constexpr ((Count % 2) == 0)
		median = (deltas[Count / 2 - 1] + median) * 0.5;

	double mean = std::accumulate(deltas.begin(), deltas.end(), 0.0) / Count;
	std::transform(deltas.begin(), deltas.end(), deltas.begin(), [mean](double x) { return x - mean; });

	double squareSum = std::inner_product(deltas.begin(), deltas.end(), deltas.begin(), 0.0);
	double standardDeviation = std::sqrt(squareSum / Count);
	double confidenceInterval = (2.57 * (standardDeviation / std::sqrt(double(Count)))) * 0.5;
	double inverseIterations = 1.0 / (double)callsPerIteration;

	std::cout << std::left << std::setw(20) << name;
	std::cout << std::right << std::fixed << std::setprecision(5);
	std::cout << ' ' << std::setw(column_width - 1) << (callsPerIteration / 1'000'000) << 'M';
	std::cout << ' ' << std::setw(column_width) << (median * inverseIterations);
	std::cout << ' ' << std::setw(column_width) << (mean * inverseIterations);
	std::cout << ' ' << std::setw(column_width) << (standardDeviation * inverseIterations);
	std::cout << ' ' << std::setw(column_width) << (confidenceInterval * inverseIterations);
	std::cout << '\n';

	//std::cout << ' ' << std::setw(6) << (callsPerIteration / 1'000'000) << "M " <<
	//	std::setw(10) << ((double(totalTime) / double(8 * callsPerIteration)) * 1'000'000) << "ns/op\n";
};