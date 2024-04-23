#pragma once
#include <cstdint>
#include <string>
#include <iostream>
#include "Profiler.h"

//Requirements
// Enable the running of a set of code repeatedly
// Only profile the piuece of code that you want to, able to ignore setup and teardown
// Repeats test a number of times, tracks min, max and average clock cycles and bandwidth
// can take in a desired bandwidth and number of runs to execute
// checks the bandwidth and runs match and does error checking if they do not

struct TestResult
{
	uint32_t startTestCount = 0;
	uint32_t completeTestCount = 0;
	uint64_t minClockCycles = ~0; // set to max value
	uint64_t maxClockCycles = 0;
	uint64_t totalClockCycles = 0;

	uint64_t minBytes = 0;
	uint64_t maxBytes = 0;
	uint64_t totalBytes = 0;

	uint64_t minPageFaults = 0;
	uint64_t maxPageFaults = 0;
	uint64_t totalPageFaults = 0;
};

struct CurrentTestStats
{
	uint64_t bytesProcessed = 0;
	uint64_t startPageFaults = 0;
	uint64_t startTime = 0;
};

struct TestParameters
{
	uint64_t expectedBytesToProcessPerTest;
	std::string testName;
	uint32_t numSecondsToFindNewResult;
};

enum class RepetitionTesterState
{
	Error,
	Executing,
	Finished
};

class RepetitionTester
{
public:
	explicit RepetitionTester(TestParameters const& testParams)
		: params(testParams)
	{
		currentTest.bytesProcessed = testParams.expectedBytesToProcessPerTest;
	}

	void BeginTest()
	{
		state = RepetitionTesterState::Executing;
		++result.startTestCount;

		currentTest.bytesProcessed = 0;
		currentTest.startPageFaults = result.totalPageFaults;
		currentTest.startTime = Profiler::ReadCpuTimer();
	}

	void EndTest(uint64_t bytesProcessed)
	{
		uint64_t currentTestEndTime = Profiler::ReadCpuTimer();
		uint64_t currentTestDuration = currentTestEndTime - currentTest.startTime;
		uint64_t currentTestEndPageFaults = Profiler::OsStats::Get().GetPageFaults();
		uint64_t currentTestPageFaults = currentTestEndPageFaults - currentTest.startPageFaults;

		currentTest.bytesProcessed = bytesProcessed;
		result.totalBytes += currentTest.bytesProcessed;
		result.totalPageFaults += currentTestPageFaults;
		result.totalClockCycles += currentTestDuration;
		clockCyclesSinceMinUpdated += currentTestDuration;
		currentTest.startTime = 0;
		currentTest.startPageFaults = 0;

		++result.completeTestCount;

		if (currentTestDuration < result.minClockCycles)
		{
			result.minClockCycles = currentTestDuration;
			result.minBytes = currentTest.bytesProcessed;
			result.minPageFaults = currentTestPageFaults;
			clockCyclesSinceMinUpdated = 0;

			//PrintTime("New Min found", result.minClockCycles, 0, 0);
		}

		if (currentTestDuration > result.maxClockCycles)
		{
			result.maxClockCycles = currentTestDuration;
			result.maxBytes = currentTest.bytesProcessed;
			result.maxPageFaults = currentTestPageFaults;
		}
	}

	void PushError(std::string const& errorMessage)
	{
		std::cout << "Error: " << errorMessage << "\n";
		state = RepetitionTesterState::Error;
	}

	bool IsTesting()
	{
		if (state == RepetitionTesterState::Executing)
		{
			if (result.startTestCount != result.completeTestCount)
			{
				PushError("Unbalanced test starts and ends");
			}

			if (currentTest.bytesProcessed != params.expectedBytesToProcessPerTest)
			{
				PushError("Processed bytes mismatched");
			}

			uint64_t secondsSinceMinUpdated = clockCyclesSinceMinUpdated / Profiler::CpuStats::Get().k_CpuFrequencyHz;
			if (secondsSinceMinUpdated >= params.numSecondsToFindNewResult)
			{
				state = RepetitionTesterState::Finished;
			}
		}

		return state == RepetitionTesterState::Executing ? true : false;
	}

	static void PrintTime(std::string const& testName, uint64_t clockCycles, uint64_t byteCount, uint64_t pageFaults)
	{
		double seconds = (double)clockCycles / (double)Profiler::CpuStats::Get().k_CpuFrequencyHz;
		std::cout << testName << ": " << seconds << "s ";

		if (byteCount != 0)
		{
			double bandwidth = (double)byteCount / seconds;
			double bandwidthGbps = bandwidth / (1024 * 1024 * 1024);
			std::cout << bandwidthGbps << "gb/s";
		}

		if (pageFaults != 0)
		{
			std::cout << " PF: " << pageFaults;
			if (byteCount != 0)
			{
				double faultsPerKbWritten = (double)byteCount / ((double)pageFaults * 1024.0);
				std::cout << "(" << faultsPerKbWritten << "kb/fault)";
			}
		}

		std::cout << "\n";
	}

	void PrintResults() const
	{
		std::cout << params.testName << ":\n";
		std::cout << "\t";
		uint64_t bytesPerTest = result.totalBytes / result.completeTestCount;
		PrintTime("min", result.minClockCycles, bytesPerTest, result.minPageFaults);

		std::cout << "\t";
		PrintTime("max", result.maxClockCycles, bytesPerTest, result.maxPageFaults);

		std::cout << "\t";
		uint64_t avgCycles = result.totalClockCycles / result.completeTestCount;
		uint64_t avgFaults = result.totalPageFaults / result.completeTestCount;
		PrintTime("avg", avgCycles, bytesPerTest, avgFaults);
	}

private:
	TestParameters params;
	TestResult result;
	CurrentTestStats currentTest;

	uint64_t clockCyclesSinceMinUpdated = 0;
	RepetitionTesterState state = RepetitionTesterState::Executing;

};