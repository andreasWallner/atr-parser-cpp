cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

if(PROJECT_NAME)
	set(ATR_IS_ROOT OFF)
else()
	set(ATR_IS_ROOT ON)
endif()

project(atr CXX)
set(CMAKE_CXX_STANDARD 17)

option(ATR_ENABLE_TESTING "Enable build of ATR tests" ${ATR_IS_ROOT})

include(FetchContent)
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2
	GIT_TAG v3.0.0-preview3
)
FetchContent_Declare(
	MSGSL
	GIT_REPOSITORY https://github.com/microsoft/GSL
	GIT_TAG v3.1.0
)

FetchContent_GetProperties(MSGSL)
if(NOT msgsl_POPULATED)
	FetchContent_Populate(MSGSL)
	add_subdirectory(${msgsl_SOURCE_DIR} ${msgsl_BINARY_DIR})
endif()

add_library(atr STATIC
	src/atr.cpp
	src/receive.cpp
)
target_include_directories(atr PUBLIC include)
target_link_libraries(atr PUBLIC Microsoft.GSL::GSL)
target_compile_features(atr PUBLIC cxx_std_17)

if(ATR_ENABLE_TESTING)
	FetchContent_GetProperties(Catch2)
	if(NOT catch2_POPULATED)
		FetchContent_Populate(Catch2)
		add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
	endif()
	enable_testing()
	add_executable(test_atr
		test/test_atr.cpp
		test/test_receive.cpp
	)
	add_test(atr test_atr)
	target_link_libraries(test_atr atr Catch2::Catch2WithMain)
endif()
