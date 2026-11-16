cmake_minimum_required(VERSION 3.16)
project(PSBBVGames LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_library(psbbv_games_catalogue src/Catalogue.cpp src/MetadataParser.cpp)
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
