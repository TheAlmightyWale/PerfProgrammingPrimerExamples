include(FetchContent)

FetchContent_Declare(
	nanobench
	GIT_REPOSITORY https://github.com/martinus/nanobench.git 
	GIT_TAG v4.3.11
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nanobench)

FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
	DOWNLOAD_EXTRACT_TIMESTAMP true
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(Examples "Examples.cpp" "Profiler.h" "RepetitionTester.h")
set_property(TARGET Examples PROPERTY CXX_STANDARD 20)
target_link_libraries(Examples PRIVATE nanobench gtest_main)