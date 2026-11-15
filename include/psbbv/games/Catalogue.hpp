#pragma once

#include <cstddef>
#include <cstdint>

namespace psbbv::games {

constexpr std::size_t MaxGames = 128;
constexpr std::size_t TitleIdCapacity = 10;
constexpr std::size_t TitleCapacity = 129;
constexpr std::size_t CategoryCapacity = 33;
constexpr std::size_t VersionCapacity = 25;

struct GameRecord final {
    char titleId[TitleIdCapacity]{};
    char title[TitleCapacity]{};
    char category[CategoryCapacity]{};
    char version[VersionCapacity]{};
    std::uint64_t lastPlayedUnix = 0;
    bool favorite = false;
};

enum class CatalogueError : std::uint8_t {
    None,
    InvalidTitleId,
    EmptyTitle,
    FieldTooLong,
    Full
};

enum class SortOrder : std::uint8_t {
    TitleAscending,
    RecentlyPlayed
};

class Catalogue final {
public:
    Catalogue() noexcept;
    CatalogueError upsert(const GameRecord& record) noexcept;
    bool remove(const char* titleId) noexcept;
    bool setFavorite(const char* titleId, bool favorite) noexcept;
    bool markPlayed(const char* titleId, std::uint64_t timestamp) noexcept;

    const GameRecord* find(const char* titleId) const noexcept;
    const GameRecord* at(std::size_t index) const noexcept;
    std::size_t size() const noexcept;
    std::size_t search(const char* query, const GameRecord** output, std::size_t capacity) const noexcept;
    std::size_t favorites(const GameRecord** output, std::size_t capacity) const noexcept;
    std::size_t sorted(SortOrder order, const GameRecord** output, std::size_t capacity) const noexcept;

private:
    GameRecord records_[MaxGames];
    std::size_t size_;
};

bool isValidTitleId(const char* titleId) noexcept;

} // namespace psbbv::games
