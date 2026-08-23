cmake_minimum_required(VERSION 4.2)
project(azula LANGUAGES C CXX DESCRIPTION "Thz Azula Ty Azote")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(Boost_NO_BOOST_CMAKE ON)
set(Boost_NO_SYSTEM_PATHS ON)
set(BOOST_ROOT "/opt/boost-1.91")
set(Boost_ROOT "/opt/boost-1.91")

find_package(Boost 1.91 REQUIRED COMPONENTS program_options fiber)

add_executable(azula main.cpp)


set(TY_AZOTE
    azula-core/libtyazote/ty_azote_core.c
    azula-core/libtyazote/ty_azote_core.h
    azula-engine/libtyazote/ty_azote_engine.c
    azula-engine/libtyazote/ty_azote_engine.h
    azula-crypto/libtyazote/ty_azote_crypto.c
    azula-crypto/libtyazote/ty_azote_crypto.h
    azula-support/libtyazote/ty_azote_support.h
    azula-support/libtyazote/ty_azote_support.c
    azula-services/libtyazote/ty_azote_services.h
    azula-services/libtyazote/ty_azote_services.c
    azula-scan/libtyazote/ty_azote_scan.h
    azula-scan/libtyazote/ty_azote_scan.c
    azula-proto/libtyazote/ty_azote_proto.h
    azula-proto/libtyazote/ty_azote_proto.c
    azula-macros/libtyazote/ty_azote_macros.h
    azula-macros/libtyazote/ty_azote_macros.c
    azula-iostream/libtyazote/ty_azote_iostream.h
    azula-iostream/libtyazote/ty_azote_iostream.c
    azula-api/libtyazote/ty_azote_api.h
    azula-api/libtyazote/ty_azote_api.c
)


add_library(ty_azote_static STATIC ${TY_AZOTE})
add_library(ty_azote_shared SHARED ${TY_AZOTE})

target_include_directories(ty_azote_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(ty_azote_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest) 

target_include_directories(azula PRIVATE ${Boost_INCLUDE_DIRS})

target_compile_options(azula PRIVATE -stdlib=libc++)
target_link_options(azula PRIVATE -stdlib=libc++)

target_link_libraries(azula PRIVATE Boost::program_options Boost::fiber)