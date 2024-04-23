#pragma once

#define NameConcat2(A,B) A##B
#define NameConcat(A, B) NameConcat2(A,B)
#define ProfileBlock(functionName, lineNumber, bytesProcessed) Profiler::ScopedProfiler NameConcat(Block,__LINE__)(functionName, lineNumber, bytesProcessed)
#define ProfileScope ProfileBlock(__func__, __LINE__, 0)
#define ProfileScopeThroughput(bytesProcessed) ProfileBlock(__func__, __LINE__, bytesProcessed)
#define ProfileLabelledScope(label) ProfileBlock(label, __LINE__, 0)
#define ProfileLabelledScopeThroughput(label, bytesProcessed) ProfileBlock(label, __LINE__, bytesProcessed)
#define PrintProfilingResults Profiler::Profiler::PrintResults()


#include <cstdint>
#include <optional>
#include <unordered_map>
#include <stack>
#include <string_view>
#include <iostream>

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <Psapi.h>

namespace Profiler
{

	static uint64_t GetOsTimerFrequency()
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return freq.QuadPart;
	}

	static uint64_t ReadOsTimer()
	{
		LARGE_INTEGER val;
		QueryPerformanceCounter(&val);
		return val.QuadPart;
	}

	static uint64_t ReadOsPageFaults(HANDLE processHandle)
	{
		PROCESS_MEMORY_COUNTERS_EX memoryCounters{};
		memoryCounters.cb = sizeof(memoryCounters);
		GetProcessMemoryInfo(processHandle, (PROCESS_MEMORY_COUNTERS*)&memoryCounters, sizeof(memoryCounters));

		return memoryCounters.PageFaultCount;
	}


#else //_WIN32

#include <x86intrin.h>
#include <sys/time.h>

namespace Profiler
{

	static uint64_t GetOSTimerFreq(void)
	{
		return 1000000;
	}

	static uint64_t ReadOSTimer(void)
	{
		timeval Value;
		gettimeofday(&Value, 0);

		u64 Result = GetOSTimerFreq() * (u64)Value.tv_sec + (u64)Value.tv_usec;
		return Result;
	}

#endif // _WIN32

	inline uint64_t ReadCpuTimer()
	{
		// This is for x86 only, would need to replace this for ARM or WASM
		return __rdtsc();
	}

	class CpuStats
	{
	public:
		static CpuStats& Get()
		{
			static CpuStats instance;
			return instance;
		}
		uint64_t const k_CpuFrequencyHz;

	private:
		uint64_t CalculateCpuFrequency(uint64_t timeToSampleForMs) const noexcept
		{
			uint64_t osFreq = GetOsTimerFrequency();
			uint64_t cpuStart = ReadCpuTimer();
			uint64_t osStart = ReadOsTimer();

			uint64_t osEnd = 0;
			uint64_t osElapsed = 0;
			uint64_t osWaitTime = osFreq * timeToSampleForMs / 1000;
			while (osElapsed < osWaitTime)
			{
				osEnd = ReadOsTimer();
				osElapsed = osEnd - osStart;
			}

			uint64_t cpuEnd = ReadCpuTimer();
			uint64_t cpuElapsed = cpuEnd - cpuStart;

			uint64_t cpuFreq = 0;
			if (osElapsed)
			{
				cpuFreq = osFreq * cpuElapsed / osElapsed;
			}

			return cpuFreq;
		}

		CpuStats() : k_CpuFrequencyHz(CalculateCpuFrequency(100 /*Sample for 100 ms*/))
		{}

		~CpuStats() = default;

		CpuStats(CpuStats const&) = delete;
		CpuStats& operator=(CpuStats const&) = delete;

	};

	class OsStats
	{
	public:

		static OsStats& Get()
		{
			static OsStats instance;
			return instance;
		}

		uint64_t GetPageFaults()
		{
			return ReadOsPageFaults(processHandle);
		}

	private:
		OsStats() = default;
		~OsStats() = default;

		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());

		OsStats(OsStats const&) = delete;
		OsStats& operator=(OsStats const&) = delete;
	};

	struct ProfileResult
	{
		uint64_t totalElapsedTime = 0;
		uint64_t childrenTotalElapsedTime = 0;
		uint64_t rootElapsedTime = 0;
		uint64_t hitCount = 0;
		uint64_t bytesProcessed = 0;

		inline uint64_t ChildExclusiveDuration() const { return totalElapsedTime - childrenTotalElapsedTime; }
	};


	class ProfilerResultsHolder
	{
	public:
		static ProfilerResultsHolder& Get()
		{
			static ProfilerResultsHolder instance;
			return instance;
		}

		std::unordered_map<std::string, ProfileResult> const& GetResults() const
		{
			return results;
		}

		ProfileResult* GetResult(std::string const& key)
		{
			results.try_emplace(key);

			return &results.at(key);
		}

		uint64_t GetTotalTime()
		{
			if (totalTimeSampled == 0)
			{
				for (auto const& [key, result] : results)
				{
					totalTimeSampled += result.ChildExclusiveDuration();
				}
			}

			return totalTimeSampled;
		}

		void PushProfilerLabel(std::string const& label)
		{
			profilerStack.push(label);
		}

		void PopProfilerLabel()
		{
			profilerStack.pop();
		}

		std::optional<ProfileResult*> GetParentProfilerResult()
		{
			if (profilerStack.size() < 2) return std::nullopt;

			auto currentLabel = profilerStack.top();
			profilerStack.pop();
			auto parentLabel = profilerStack.top();
			profilerStack.push(currentLabel);

			return GetResult(parentLabel);
		}

		void PrintResults()
		{
			CpuStats& cpuStats = CpuStats::Get();
			uint64_t totalTime = GetTotalTime();

			if (cpuStats.k_CpuFrequencyHz)
			{
				std::cout << "Total time: " << (double)totalTime / (double)cpuStats.k_CpuFrequencyHz << "s CPU Freq: " << cpuStats.k_CpuFrequencyHz << "hz\n\n";
			}

			for (auto const& [label, result] : results)
			{
				PrintTimeElapsed(label, totalTime, result);
			}
		}

	private:
		ProfilerResultsHolder() = default;

		void PrintTimeElapsed(std::string const& timeSectionName, uint64_t totalTime, ProfileResult const& result)
		{
			double percent = 100.0 * ((double)result.ChildExclusiveDuration() / (double)totalTime);
			std::cout << timeSectionName << "[" << result.hitCount << "]" << ": " << result.ChildExclusiveDuration() << " cycles" << " (" << percent << "%)\n";

			if (result.childrenTotalElapsedTime != 0)
			{
				double percentWithChildren = 100.0 * ((double)result.rootElapsedTime / (double)totalTime);
				std::cout << "\tChild Inclusive Time : " << result.rootElapsedTime << " cycles(" << percentWithChildren << "%)\n";
			}

			if (result.bytesProcessed != 0)
			{
				double k_megabyte = 1024.0 * 1024.0;
				double k_gigabyte = k_megabyte * 1024.0;
				double secondsElapsed = (double)result.totalElapsedTime / (double)CpuStats::Get().k_CpuFrequencyHz;
				double bytesPerSecond = (double)result.bytesProcessed / secondsElapsed;
				double megabytesProcessed = (double)result.bytesProcessed / k_megabyte;
				double gigabytesPerSecond = bytesPerSecond / k_gigabyte;

				std::cout << "\tThroughput: " << megabytesProcessed << "mb " << gigabytesPerSecond << "gb/s \n";
			}

			std::cout << "\n";
		}

		std::unordered_map<std::string, ProfileResult> results;
		uint64_t totalTimeSampled = 0;
		std::optional<uint64_t> activeProfileResultIndex = std::nullopt;
		std::stack<std::string> profilerStack;
	};


	class Profiler
	{
	public:
		inline void Begin(const char* functionName, int lineNumber, uint64_t bytesProcessed)
		{
			resultLabel = std::string(functionName);
			resultLabel += std::to_string(lineNumber);

			ProfileResult* res = ProfilerResultsHolder::Get().GetResult(resultLabel);
			res->bytesProcessed += bytesProcessed;

			start = ReadCpuTimer();
			ProfilerResultsHolder::Get().PushProfilerLabel(resultLabel);
		}

		inline void End() {
			ProfileResult* res = ProfilerResultsHolder::Get().GetResult(resultLabel);
			uint64_t elapsedTime = ReadCpuTimer() - start;
			res->totalElapsedTime += elapsedTime;
			res->rootElapsedTime = elapsedTime;
			++res->hitCount;

			if (auto oParentRes = ProfilerResultsHolder::Get().GetParentProfilerResult(); oParentRes)
			{
				ProfileResult* parentRes = *oParentRes;
				parentRes->childrenTotalElapsedTime += elapsedTime;
			}

			ProfilerResultsHolder::Get().PopProfilerLabel();
		}

		static void PrintResults()
		{
			ProfilerResultsHolder::Get().PrintResults();
		}

		inline ProfileResult const& GetResult() { return *ProfilerResultsHolder::Get().GetResult(resultLabel); }

	private:
		std::string resultLabel;
		uint64_t start;
	};

	class ScopedProfiler
	{
	public:
		explicit ScopedProfiler(char const* functionName, int lineNumber, uint64_t bytesProcessed = 0)
		{
			profiler.Begin(functionName, lineNumber, bytesProcessed);
		}

		~ScopedProfiler()
		{
			profiler.End();
		}

		ScopedProfiler(ScopedProfiler const&) = delete;
		ScopedProfiler(ScopedProfiler const&&) = delete;
		ScopedProfiler& operator=(ScopedProfiler const&) = delete;
		ScopedProfiler& operator=(ScopedProfiler const&&) = delete;
	private:
		Profiler profiler;
	};


} // namespace Profiler

