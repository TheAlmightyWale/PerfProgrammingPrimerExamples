#include <gtest/gtest.h>
#include <nanobench.h>
#include <random>
#include "RepetitionTester.h"

namespace Bench = ankerl::nanobench;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Execution port examples
// These functions just call mov sequentially to either read or store data.
// Each function calls mov a number of times in sequence. As we call different consequtive sequences of mov we will eventually see
// a hard limit form on some sequential count. This indicates that the execution ports are maxed out as the CPU cannot parallelize these
// instructions further, even though it theoretically could execute more of these functions in parallel as there are no dependency chains
// between them.
extern "C" void Mov1x(uint64_t count, uint8_t * data);
extern "C" void Mov2x(uint64_t count, uint8_t * data);
extern "C" void Mov3x(uint64_t count, uint8_t * data);
extern "C" void Mov4x(uint64_t count, uint8_t * data);
extern "C" void Store1x(uint64_t count, uint8_t * data);
extern "C" void Store2x(uint64_t count, uint8_t * data);
extern "C" void Store3x(uint64_t count, uint8_t * data);
extern "C" void Store4x(uint64_t count, uint8_t * data);
#pragma comment(lib, "movs")

using RepetitionTestFn = std::function<void(uint64_t count, uint8_t* pData)>;
uint32_t const k_gb = 1024 * 1024 * 1024;

void RepetitionTest(std::string const& testName, RepetitionTestFn const& fn)
{
	TestParameters params{
		.expectedBytesToProcessPerTest = k_gb,
		.testName = testName,
		.numSecondsToFindNewResult = 2
	};

	RepetitionTester tester(params);

	while (tester.IsTesting())
	{
		std::vector<uint8_t> mem;
		mem.resize(k_gb);

		tester.BeginTest();
		fn(mem.size(), mem.data());
		tester.EndTest(k_gb);
	}

	tester.PrintResults();
}

//test
//allocate memory buffer that is large enough to hit main memory
//Count bytes read per cycle
//ouput result

//TEST(ExecutionPorts, mov1x)
//{
//	RepetitionTest("mov1x", &Mov1x);
//}
//
//TEST(ExecutionPorts, mov2x)
//{
//	RepetitionTest("mov2x", &Mov2x);
//}
//
//TEST(ExecutionPorts, mov3x)
//{
//	RepetitionTest("mov3x", &Mov3x);
//}
//
//TEST(ExecutionPorts, mov4x)
//{
//	RepetitionTest("mov4x", &Mov4x);
//}
//
//TEST(ExecutionPorts, store1x)
//{
//	RepetitionTest("store1x", &Store1x);
//}
//
//TEST(ExecutionPorts, store2x)
//{
//	RepetitionTest("store2x", &Store2x);
//}
//
//TEST(ExecutionPorts, store3x)
//{
//	RepetitionTest("store3x", &Store3x);
//}
//
//TEST(ExecutionPorts, store4x)
//{
//	RepetitionTest("store4x", &Store4x);
//}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Structure of Arrays example
// We perform the same operation on a large piece of data and show how arranging that data in a way that is conducive to the 
// operations being performed on it can provide great performance benefits

constexpr uint32_t k_randomSeed = 238397;
constexpr uint32_t k_arraySize = 100'000;

inline uint32_t GenerateInRange(std::mt19937& generator, uint32_t start, uint32_t end)
{
	std::uniform_int_distribution<uint32_t>dist(start, end);
	return dist(generator);
}

struct Junk
{
	uint64_t a;
	uint64_t b;
	uint64_t c;
	uint64_t d;
};

struct kv
{
	uint32_t key;
	Junk j;
};

bool Search(kv* data, uint32_t target)
{
	for (uint32_t i = 0; i < k_arraySize; i++)
	{
		if (data[i].key == target) return true;
	}

	return false;
}

struct kv_soa
{
	std::vector<uint32_t> keys;
	std::vector<Junk> j;
};

bool Search(uint32_t* keyArray, uint32_t target)
{
	for (uint32_t i = 0; i < k_arraySize; i++)
	{
		if (keyArray[i] == target) return true;
	}

	return false;
}

TEST(StructureOfArrays, ArrayOfStructs)
{
	std::vector<kv> kvs;
	kvs.resize(k_arraySize);
	for (int i = 0; i < k_arraySize; i++)
	{
		kvs[i].key = i;
	}

	std::mt19937 generator(k_randomSeed);

	Bench::Bench().minEpochIterations(1000).run("ArrayOfStructs", [&] {
		uint32_t target = GenerateInRange(generator, 0, k_arraySize - 1);
		Bench::doNotOptimizeAway(Search(kvs.data(), target));
	});

}

TEST(StructureOfArrays, StructureOfArrays)
{
	kv_soa kvs;
	kvs.keys.resize(k_arraySize);
	kvs.j.resize(k_arraySize);
	for (int i = 0; i < k_arraySize; i++)
	{
		kvs.keys[i] = i;
	}

	std::mt19937 generator(k_randomSeed);

	Bench::Bench().minEpochIterations(1000).run("StructOfArrays", [&] {
		uint32_t target = GenerateInRange(generator, 0, k_arraySize - 1);
		Bench::doNotOptimizeAway(Search(kvs.keys.data(), target));
	});
}

struct kvp
{
	uint32_t key;
	std::shared_ptr<uint32_t> pData;
};

bool Search(kvp* data, uint32_t target)
{
	for (uint32_t i = 0; i < k_arraySize; i++)
	{
		if (data[i].key == target) return true;
	}

	return false;
}

struct kvp_soa
{
	std::vector<uint32_t> keys;
	std::vector<std::shared_ptr<uint32_t>> data;
};

TEST(StructureOfArrays, ArrayOfStructsPointer)
{
	std::vector<kvp> kvs;
	kvs.resize(k_arraySize);
	for (int i = 0; i < k_arraySize; i++)
	{
		kvs[i].key = i;
		kvs[i].pData = std::make_shared<uint32_t>(i);
	}

	std::mt19937 generator(k_randomSeed);

	Bench::Bench().minEpochIterations(1000).run("ArrayOfStructs", [&] {
		uint32_t target = GenerateInRange(generator, 0, k_arraySize - 1);
		Bench::doNotOptimizeAway(Search(kvs.data(), target));
	});
}

TEST(StructureOfArrays, StructureOfArraysPointer)
{
	kvp_soa kvs;
	kvs.keys.resize(k_arraySize);
	kvs.data.resize(k_arraySize);
	for (int i = 0; i < k_arraySize; i++)
	{
		kvs.keys[i] = i;
		kvs.data[i] = std::make_shared<uint32_t>(i);
	}

	std::mt19937 generator(k_randomSeed);
	Bench::Bench().minEpochIterations(1000).run("StructOfArrays", [&] {
		uint32_t target = GenerateInRange(generator, 0, k_arraySize - 1);
		Bench::doNotOptimizeAway(Search(kvs.keys.data(), target));
	});
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Hoisting Example
//We want to help our compilers by explicitly hoisting out loop-invariants
// we also want to reduce branch mis-predictions by arranging our data and processing in such a way that we minimise branches taken
// and if we do have to have a low level branch then the data that we are branching on helps the branch predictor as much as possible
// Ideally we remove branches in general and operate with masking operations

//low-level unsorted branch

//sorted branch

//higher level branch (essentially sorted)

//masked operations


//std_polymorphic_allocator
//pool, monotonic examples

//pre-allocate pool
//do a bunch of adds and removes

//vanilla vector compare

//monotonic
//compare destruction



