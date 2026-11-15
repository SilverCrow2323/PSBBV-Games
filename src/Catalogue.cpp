#include "psbbv/games/Catalogue.hpp"

#include <cctype>
#include <cstring>

namespace psbbv::games {
namespace {

bool copyField(char* destination, std::size_t capacity, const char* source) noexcept {
    if (source == nullptr) source = "";
    const std::size_t length = std::strlen(source);
    if (length >= capacity) return false;
    std::memcpy(destination, source, length + 1);
    return true;
}

bool containsInsensitive(const char* text, const char* query) noexcept {
    if (query == nullptr || query[0] == '\0') return true;
    if (text == nullptr) return false;
    const std::size_t queryLength = std::strlen(query);
    for (std::size_t i = 0; text[i] != '\0'; ++i) {
        std::size_t j = 0;
        while (j < queryLength && text[i + j] != '\0' &&
               std::tolower(static_cast<unsigned char>(text[i + j])) ==
               std::tolower(static_cast<unsigned char>(query[j]))) ++j;
        if (j == queryLength) return true;
    }
    return false;
}

} // namespace

Catalogue::Catalogue() noexcept : records_{}, size_(0) {}

bool isValidTitleId(const char* titleId) noexcept {
    if (titleId == nullptr || std::strlen(titleId) != 9) return false;
    for (std::size_t i = 0; i < 9; ++i) {
        const unsigned char ch = static_cast<unsigned char>(titleId[i]);
        if (!std::isalnum(ch)) return false;
    }
    return true;
}

CatalogueError Catalogue::upsert(const GameRecord& record) noexcept {
    if (!isValidTitleId(record.titleId)) return CatalogueError::InvalidTitleId;
    if (record.title[0] == '\0') return CatalogueError::EmptyTitle;
    if (std::strlen(record.title) >= TitleCapacity ||
        std::strlen(record.category) >= CategoryCapacity ||
        std::strlen(record.version) >= VersionCapacity) return CatalogueError::FieldTooLong;

    std::size_t index = size_;
    for (std::size_t i = 0; i < size_; ++i) {
        if (std::strcmp(records_[i].titleId, record.titleId) == 0) {
            index = i;
            break;
        }
    }
    if (index == size_ && size_ == MaxGames) return CatalogueError::Full;

    GameRecord clean{};
    if (!copyField(clean.titleId, sizeof(clean.titleId), record.titleId) ||
        !copyField(clean.title, sizeof(clean.title), record.title) ||
        !copyField(clean.category, sizeof(clean.category), record.category) ||
        !copyField(clean.version, sizeof(clean.version), record.version)) {
        return CatalogueError::FieldTooLong;
    }
    clean.lastPlayedUnix = record.lastPlayedUnix;
    clean.favorite = record.favorite || (index < size_ && records_[index].favorite);
    records_[index] = clean;
    if (index == size_) ++size_;
    return CatalogueError::None;
}

bool Catalogue::remove(const char* titleId) noexcept {
    if (titleId == nullptr) return false;
    for (std::size_t i = 0; i < size_; ++i) {
        if (std::strcmp(records_[i].titleId, titleId) == 0) {
            for (std::size_t j = i + 1; j < size_; ++j) records_[j - 1] = records_[j];
            --size_;
            records_[size_] = GameRecord{};
            return true;
        }
    }
    return false;
}

bool Catalogue::setFavorite(const char* titleId, bool favorite) noexcept {
    if (titleId == nullptr) return false;
    for (std::size_t i = 0; i < size_; ++i) {
        if (std::strcmp(records_[i].titleId, titleId) == 0) {
            records_[i].favorite = favorite;
            return true;
        }
    }
    return false;
}

bool Catalogue::markPlayed(const char* titleId, std::uint64_t timestamp) noexcept {
    if (titleId == nullptr) return false;
    for (std::size_t i = 0; i < size_; ++i) {
        if (std::strcmp(records_[i].titleId, titleId) == 0) {
            records_[i].lastPlayedUnix = timestamp;
            return true;
        }
    }
    return false;
}

const GameRecord* Catalogue::find(const char* titleId) const noexcept {
    if (titleId == nullptr) return nullptr;
    for (std::size_t i = 0; i < size_; ++i)
        if (std::strcmp(records_[i].titleId, titleId) == 0) return &records_[i];
    return nullptr;
}

const GameRecord* Catalogue::at(std::size_t index) const noexcept {
    return index < size_ ? &records_[index] : nullptr;
}

std::size_t Catalogue::size() const noexcept { return size_; }

std::size_t Catalogue::search(const char* query, const GameRecord** output, std::size_t capacity) const noexcept {
    if (output == nullptr || capacity == 0) return 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < size_ && count < capacity; ++i) {
        if (containsInsensitive(records_[i].title, query) ||
            containsInsensitive(records_[i].titleId, query) ||
            containsInsensitive(records_[i].category, query)) output[count++] = &records_[i];
    }
    return count;
}

std::size_t Catalogue::favorites(const GameRecord** output, std::size_t capacity) const noexcept {
    if (output == nullptr || capacity == 0) return 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < size_ && count < capacity; ++i)
        if (records_[i].favorite) output[count++] = &records_[i];
    return count;
}

std::size_t Catalogue::sorted(SortOrder order, const GameRecord** output, std::size_t capacity) const noexcept {
    if (output == nullptr || capacity == 0) return 0;
    const std::size_t count = size_ < capacity ? size_ : capacity;
    for (std::size_t i = 0; i < count; ++i) output[i] = &records_[i];
    for (std::size_t i = 1; i < count; ++i) {
        const GameRecord* value = output[i];
        std::size_t j = i;
        while (j > 0) {
            bool before = false;
            if (order == SortOrder::TitleAscending)
                before = std::strcmp(value->title, output[j - 1]->title) < 0;
            else
                before = value->lastPlayedUnix > output[j - 1]->lastPlayedUnix;
            if (!before) break;
            output[j] = output[j - 1];
            --j;
        }
        output[j] = value;
    }
    return count;
}

} // namespace psbbv::games
