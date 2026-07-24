cmake_minimum_required(VERSION 4.2)
project(azula CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(Boost_NO_SYSTEM_PATHS ON)
set(BOOST_ROOT "/opt/boost-1.91")
set(Boost_ROOT "/opt/boost-1.91")


find_package(Boost 1.91 REQUIRED COMPONENTS
        filesystem
        program_options
        fiber
)


add_executable(azula main.cpp
        azula-crypto/lib/permutation.hh
        azula-crypto/lib/blackrock.hh
        azula-crypto/lib/blackrock.cc
        azula-crypto/lib/blackrock2.cc
        azula-crypto/lib/blackrock2.hh
        azula-crypto/tests/test_blackrock.cc
        azula-crypto/lib/primes.cc
        azula-crypto/lib/primes.hh
        azula-crypto/lib/lcg.cc
        azula-crypto/lib/lcg.hh)



include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest) 

target_include_directories(azula PRIVATE ${Boost_INCLUDE_DIRS})

target_compile_options(azula PRIVATE -stdlib=libc++)
target_link_options(azula PRIVATE -stdlib=libc++)


target_link_libraries(azula PRIVATE
        Boost::filesystem
        Boost::program_options
        Boost::fiber
)