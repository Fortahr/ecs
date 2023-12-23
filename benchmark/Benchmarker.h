#pragma once

#include <iomanip>
#include <iostream>
#include <numeric>

class Benchmarker
{
private:
	static constexpr size_t column_width = 12;
	static inline size_t first_column_width = 24;
	static inline double biggest_measurement = 0.0;

	using sub_test_result_t = std::tuple<std::string, size_t, double, double, double, double>;
	using test_result_t = std::tuple<std::string, std::vector<sub_test_result_t>, bool>;

	static inline std::vector<test_result_t> tests;

public:
	template<typename _Func>
	struct sub_run
	{
		std::string_view name;
		_Func func;
		const size_t& count;

		constexpr sub_run<_Func>(std::string_view name, _Func func, const size_t& count)
			: name(name)
			, func(func)
			, count(count)
		{ }
	};

private:
	template <size_t Count = 20, typename Func>
	static sub_test_result_t benchmark(sub_run<Func> subTest);

	static void print_measurement(std::ostream& cout, double measurement, double division);

public:
	template <size_t Count = 20, typename Func>
	static void benchmark(std::string_view name, Func func, const size_t&);

	template <size_t Count = 20, typename... Func>
	static void benchmark(std::string_view name, sub_run<Func>... subTests);

	static void print_results();

	static std::string format_count(size_t count);
};

template <size_t Count, typename Func>
inline auto Benchmarker::benchmark(sub_run<Func> subTest) -> sub_test_result_t
{
	std::array<double, Count> deltas;

	std::chrono::steady_clock::time_point start, end;

	for (size_t i = 0; i < Count; ++i)
	{
		start = std::chrono::high_resolution_clock::now();

		constexpr auto& f = subTest.func;
		f();

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
	double inverseIterations = 1.0 / (double)subTest.count;

	median *= inverseIterations;
	mean *= inverseIterations;

	first_column_width = std::max(first_column_width, subTest.name.size() + 2);
	biggest_measurement = std::max({ biggest_measurement, median, mean });

	return sub_test_result_t {
		subTest.name,
		subTest.count,
		median,
		mean,
		standardDeviation * inverseIterations,
		confidenceInterval * inverseIterations
	};
};

template <size_t Count, typename Func>
inline void Benchmarker::benchmark(std::string_view name, Func func, const size_t& callsPerIteration)
{
	std::cout << "Running benchmark \"" << name << "\"\n";

	auto& test = tests.emplace_back(name, std::vector<sub_test_result_t>{}, false);
	std::get<1>(test).push_back(benchmark<Count>(sub_run<Func>(name, func, callsPerIteration)));
}

template <size_t Count, typename... Func>
inline void Benchmarker::benchmark(std::string_view name, sub_run<Func>... subTests)
{
	constexpr size_t subTestCount = sizeof...(Func);

	auto& test = tests.emplace_back(name, std::vector<sub_test_result_t>{}, true);
	std::get<1>(test).reserve(subTestCount);

	size_t i = 0;
	(([&] (auto subTest) {
		std::cout << "\33[2K" << "Running benchmark \"" << name << "\", sub run (" << ++i << " of " << subTestCount << ")\r";
		std::get<1>(test).push_back(benchmark<Count>(subTest));
		}(subTests)), ...);

	if constexpr (subTestCount > 0)
		std::cout << '\n';
}

inline void Benchmarker::print_measurement(std::ostream& cout, double measurement, double division)
{
	cout << std::right << std::fixed << std::setprecision(5) << std::setw(column_width) << (measurement / division);
}

inline std::string Benchmarker::format_count(size_t count)
{
	constexpr char postfixes[]{ '\0', 'K', 'M', 'G', 'T', 'P', 'E', 'Z' };

	char buffer[8] = { 0 };
	const char* p = postfixes;

	size_t remainder = 0;
	for (; count >= 1000; ++p)
	{
		remainder = count % 1000;
		count /= 1000;
	}

	char* b = buffer + 7;
	*--b = *p;

	if (remainder)
	{
		if (char c = (remainder /= 10) % 10) *--b = '0' + c;
		*--b = '0' + char(remainder / 10);
		*--b = '.';
	}

	*--b = '0' + count % 10;
	if (count /= 10) *--b = '0' + count % 10;
	if (count /= 10) *--b = '0' + count % 10;

	return std::string(b, 7 - (b - buffer));
}

inline void Benchmarker::print_results()
{
	std::ostringstream cout;

	cout << std::left << std::setw(first_column_width) << "Test";

	cout << std::right;
	cout << ' ' << std::setw(column_width) << "Count";
	cout << ' ' << std::setw(column_width) << "Median";
	cout << ' ' << std::setw(column_width) << "Mean";
	cout << ' ' << std::setw(column_width) << "StdDev";
	cout << ' ' << std::setw(column_width) << "CI / 2";
	cout << '\n';

	bool prevWasGroup = false;
	double measurement_thousandths = std::floor(std::log10(biggest_measurement) / 3);
	double measurement_division = std::pow(1000, measurement_thousandths);

	for (auto& test : tests)
	{
		bool curIsGroup = std::get<2>(test);

		if (prevWasGroup || curIsGroup)
			cout << '\n';

		if (curIsGroup)
			cout << std::left << std::setw(first_column_width) << std::get<0>(test) << '\n';

		for (auto& subTest : std::get<1>(test))
		{
			if (curIsGroup)
				cout << "  " << std::left << std::setw(first_column_width - 2) << std::get<0>(subTest);
			else
				cout << std::left << std::setw(first_column_width) << std::get<0>(subTest);

			cout << std::right << std::fixed << std::setprecision(5);
			cout << ' ' << format_count(std::get<1>(subTest));
			print_measurement(cout << ' ', std::get<2>(subTest), measurement_division);
			print_measurement(cout << ' ', std::get<3>(subTest), measurement_division);
			print_measurement(cout << ' ', std::get<4>(subTest), measurement_division);
			print_measurement(cout << ' ', std::get<5>(subTest), measurement_division);
			cout << '\n';
		}

		prevWasGroup = curIsGroup;
	}

	std::cout << cout.str();
}