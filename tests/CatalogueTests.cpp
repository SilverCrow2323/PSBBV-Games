#include "psbbv/games/Catalogue.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {
void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
psbbv::games::GameRecord game(const char* id, const char* title, const char* category, std::uint64_t played) {
    psbbv::games::GameRecord result{};
    std::strncpy(result.titleId, id, sizeof(result.titleId) - 1);
    std::strncpy(result.title, title, sizeof(result.title) - 1);
    std::strncpy(result.category, category, sizeof(result.category) - 1);
    std::strncpy(result.version, "01.00", sizeof(result.version) - 1);
    result.lastPlayedUnix = played;
    return result;
}
}
int main() {
    using namespace psbbv::games;
    Catalogue catalogue;
    require(isValidTitleId("PCSE00001"), "valid Vita title ID accepted");
    require(!isValidTitleId("../bad-id"), "path-like title ID rejected");
    require(catalogue.upsert(game("PCSE00001", "A Game", "Action", 10)) == CatalogueError::None,
            "first title inserted");
    require(catalogue.upsert(game("PCSE00002", "B Game", "Puzzle", 30)) == CatalogueError::None,
            "second title inserted");
    require(catalogue.size() == 2, "catalogue counts unique records");
    require(catalogue.upsert(game("PCSE00001", "A Game Updated", "Action", 20)) == CatalogueError::None,
            "existing title updates rather than duplicates");
    require(catalogue.size() == 2, "upsert preserves uniqueness");
    require(catalogue.find("PCSE00001") != nullptr, "lookup by title ID works");
    require(catalogue.setFavorite("PCSE00001", true), "favorite can be enabled");
    const GameRecord* selected[4]{};
    require(catalogue.favorites(selected, 4) == 1, "favorites query returns expected record");
    require(catalogue.search("pUzZ", selected, 4) == 1, "case-insensitive category search works");
    require(catalogue.sorted(SortOrder::RecentlyPlayed, selected, 4) == 2 &&
            std::strcmp(selected[0]->titleId, "PCSE00002") == 0, "recently played sort works");
    require(catalogue.remove("PCSE00002"), "record can be removed");
    require(catalogue.size() == 1, "remove compacts catalogue");
    require(catalogue.upsert(game("../bad-id", "Bad", "Other", 0)) == CatalogueError::InvalidTitleId,
            "invalid title IDs rejected");
    require(catalogue.upsert(game("PCSE00003", "", "Other", 0)) == CatalogueError::EmptyTitle,
            "empty titles rejected");
    std::cout << "PSBBV Games catalogue: all checks passed.\n";
}
