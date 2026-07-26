cmake_minimum_required(VERSION 4.2)
project(azula CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(Boost_NO_SYSTEM_PATHS ON)
set(BOOST_ROOT "/opt/boost-1.91")
set(Boost_ROOT "/opt/boost-1.91")


find_package(Boost 1.91 REQUIRED COMPONENTS filesystem program_options fiber)



add_executable(azula main.cpp
        azula-crypto/lib/blackrock.hh
        azula-crypto/lib/blackrock.cc
        azula-crypto/lib/siphash.cc
        azula-crypto/lib/siphash.hh
        azula-crypto/lib/base64.hh
        azula-crypto/lib/lcg.cc
        azula-crypto/lib/lcg.hh
        azula-crypto/lib/base64.hh
        azula-crypto/lib/base64.cc
        azula-crypto/lib/xring.hh
        azula-crypto/lib/xring.cc
        azula-support/lib/util-safefunc.hh
        azula-support/lib/util-safefunc.cc
        azula-support/lib/util-malloc.hh
        azula-support/lib/util-malloc.cc
        azula-support/lib/util-checksum.hh
        azula-support/lib/util-checksum.cc
        azula-support/lib/util-logger.hh
        azula-support/lib/util-logger.cc
        azula-support/lib/util-extract.hh
        azula-support/lib/util-extract.cc
        azula-support/lib/util-errormsg.hh
        azula-support/lib/util-errormsg.cc
)

add_executable(azula_crypto_tests
        azula-crypto/tests/crypto_test.cc
        azula-crypto/lib/blackrock.cc
        azula-crypto/lib/siphash.cc
        azula-crypto/lib/lcg.cc
        azula-crypto/lib/base64.cc
        azula-crypto/lib/xring.cc
)


add_executable(azula_support_tests
        azula-support/tests/test_util_safefunc.cc
        azula-support/tests/test_util_all.cc
        azula-support/lib/util-safefunc.cc
        azula-support/lib/util-malloc.cc
        azula-support/lib/util-checksum.cc
        azula-support/lib/util-logger.cc
        azula-support/lib/util-extract.cc
        azula-support/lib/util-errormsg.cc
)

target_link_libraries(azula_crypto_tests PRIVATE GTest::gtest_main GTest::gtest_main)
target_link_libraries(azula_support_tests PRIVATE GTest::gtest_main GTest::gtest_main)

include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)



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