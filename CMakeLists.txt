cmake_minimum_required(VERSION 3.16)

option(PSBBV_GAMES_BUILD_VITA_ADAPTER "Cross-compile the user-mode Vita filesystem provider" OFF)
if(PSBBV_GAMES_BUILD_VITA_ADAPTER AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
            CACHE FILEPATH "VitaSDK toolchain file")
    else()
        message(FATAL_ERROR "PSBBV_GAMES_BUILD_VITA_ADAPTER requires VITASDK.")
    endif()
endif()

project(PSBBVGames LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_library(psbbv_games_catalogue src/Catalogue.cpp src/MetadataParser.cpp src/Discovery.cpp)
target_include_directories(psbbv_games_catalogue PUBLIC include)
target_compile_options(psbbv_games_catalogue PRIVATE -Wall -Wextra -Wpedantic)

add_executable(psbbv_games_tests tests/CatalogueTests.cpp)
target_link_libraries(psbbv_games_tests PRIVATE psbbv_games_catalogue)
target_compile_options(psbbv_games_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME psbbv_games_catalogue COMMAND psbbv_games_tests)

add_executable(psbbv_games_metadata_tests tests/MetadataParserTests.cpp)
target_link_libraries(psbbv_games_metadata_tests PRIVATE psbbv_games_catalogue)
target_compile_options(psbbv_games_metadata_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME psbbv_games_metadata COMMAND psbbv_games_metadata_tests)

add_executable(psbbv_games_discovery_tests tests/DiscoveryTests.cpp)
target_link_libraries(psbbv_games_discovery_tests PRIVATE psbbv_games_catalogue)
target_compile_options(psbbv_games_discovery_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME psbbv_games_discovery COMMAND psbbv_games_discovery_tests)

if(PSBBV_GAMES_BUILD_VITA_ADAPTER)
    include("$ENV{VITASDK}/share/vita.cmake")
    add_library(psbbv_games_vita_adapter STATIC src/vita/DiscoveryVita.cpp)
    target_link_libraries(psbbv_games_vita_adapter PUBLIC psbbv_games_catalogue)
    target_compile_options(psbbv_games_vita_adapter PRIVATE -Wall -Wextra -Wpedantic)
endif()
