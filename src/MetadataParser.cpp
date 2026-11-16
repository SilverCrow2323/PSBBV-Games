#include "psbbv/games/MetadataParser.hpp"

#include <cstring>

namespace psbbv::games {
namespace {

constexpr std::size_t HeaderSize = 20;
constexpr std::size_t EntrySize = 16;
constexpr std::size_t MaxEntries = 128;
constexpr std::uint32_t PsfVersion = 0x00000101;
constexpr std::uint16_t StringFormat = 0x0204;

std::uint16_t read16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8);
}

std::uint32_t read32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

bool boundedString(const std::uint8_t* data, std::size_t length,
                   const char*& value, std::size_t& valueLength) noexcept {
    if (data == nullptr || length == 0) return false;
    const void* terminator = std::memchr(data, 0, length);
    if (terminator == nullptr) return false;
    value = reinterpret_cast<const char*>(data);
    valueLength = static_cast<const std::uint8_t*>(terminator) - data;
    return valueLength != 0;
}

bool copyString(char* destination, std::size_t capacity,
                const char* source, std::size_t length) noexcept {
    if (length == 0 || length >= capacity) return false;
    std::memcpy(destination, source, length);
    destination[length] = '\0';
    return true;
}

} // namespace

MetadataParseResult parseParamSfo(const std::uint8_t* bytes,
                                  std::size_t size,
                                  GameRecord& output) noexcept {
    if (bytes == nullptr || size < HeaderSize) return {MetadataError::InvalidArgument, 0};
    if (bytes[0] != 0x00 || bytes[1] != 'P' || bytes[2] != 'S' || bytes[3] != 'F')
        return {MetadataError::InvalidHeader, 0};
    if (read32(bytes + 4) != PsfVersion) return {MetadataError::UnsupportedVersion, 0};

    const std::size_t keyTableOffset = read32(bytes + 8);
    const std::size_t dataTableOffset = read32(bytes + 12);
    const std::size_t entryCount = read32(bytes + 16);
    if (entryCount > MaxEntries) return {MetadataError::TooManyEntries, 0};
    if (entryCount > (size - HeaderSize) / EntrySize)
        return {MetadataError::InvalidTable, 0};

    const std::size_t entriesEnd = HeaderSize + entryCount * EntrySize;
    if (keyTableOffset < entriesEnd || keyTableOffset > size ||
        dataTableOffset < keyTableOffset || dataTableOffset > size)
        return {MetadataError::InvalidTable, 0};

    const std::size_t keyTableSize = dataTableOffset - keyTableOffset;
    GameRecord parsed{};
    bool haveTitleId = false;
    bool haveTitle = false;
    bool haveCategory = false;
    bool haveVersion = false;

    for (std::size_t i = 0; i < entryCount; ++i) {
        const std::uint8_t* entry = bytes + HeaderSize + i * EntrySize;
        const std::size_t keyOffset = read16(entry);
        const std::uint16_t format = read16(entry + 2);
        const std::size_t valueLength = read32(entry + 4);
        const std::size_t valueCapacity = read32(entry + 8);
        const std::size_t valueOffset = read32(entry + 12);

        if (keyOffset >= keyTableSize)
            return {MetadataError::InvalidKey, i};
        const std::uint8_t* keyStart = bytes + keyTableOffset + keyOffset;
        const std::size_t remainingKeyBytes = keyTableSize - keyOffset;
        const void* keyEnd = std::memchr(keyStart, 0, remainingKeyBytes);
        if (keyEnd == nullptr) return {MetadataError::InvalidKey, i};
        const char* key = reinterpret_cast<const char*>(keyStart);
        const std::size_t keyLength = static_cast<const std::uint8_t*>(keyEnd) - keyStart;

        if (valueLength > valueCapacity || valueOffset > size - dataTableOffset ||
            valueLength > size - dataTableOffset - valueOffset)
            return {MetadataError::InvalidValue, i};

        const bool recognized =
            (keyLength == 8 && std::memcmp(key, "TITLE_ID", 8) == 0) ||
            (keyLength == 5 && std::memcmp(key, "TITLE", 5) == 0) ||
            (keyLength == 8 && std::memcmp(key, "CATEGORY", 8) == 0) ||
            (keyLength == 7 && std::memcmp(key, "APP_VER", 7) == 0);
        if (!recognized) continue;
        if (format != StringFormat) return {MetadataError::InvalidValue, i};

        const char* value = nullptr;
        std::size_t length = 0;
        if (!boundedString(bytes + dataTableOffset + valueOffset, valueLength, value, length))
            return {MetadataError::InvalidValue, i};

        if (keyLength == 8 && std::memcmp(key, "TITLE_ID", 8) == 0) {
            if (haveTitleId) return {MetadataError::DuplicateField, i};
            if (length != 9 || !copyString(parsed.titleId, sizeof(parsed.titleId), value, length))
                return {MetadataError::InvalidTitleId, i};
            haveTitleId = true;
        } else if (keyLength == 5 && std::memcmp(key, "TITLE", 5) == 0) {
            if (haveTitle) return {MetadataError::DuplicateField, i};
            if (!copyString(parsed.title, sizeof(parsed.title), value, length))
                return {MetadataError::FieldTooLong, i};
            haveTitle = true;
        } else if (keyLength == 8 && std::memcmp(key, "CATEGORY", 8) == 0) {
            if (haveCategory) return {MetadataError::DuplicateField, i};
            if (!copyString(parsed.category, sizeof(parsed.category), value, length))
                return {MetadataError::FieldTooLong, i};
            haveCategory = true;
        } else if (keyLength == 7 && std::memcmp(key, "APP_VER", 7) == 0) {
            if (haveVersion) return {MetadataError::DuplicateField, i};
            if (!copyString(parsed.version, sizeof(parsed.version), value, length))
                return {MetadataError::FieldTooLong, i};
            haveVersion = true;
        }
    }

    if (!haveTitleId) return {MetadataError::MissingTitleId, 0};
    if (!haveTitle) return {MetadataError::MissingTitle, 0};
    if (!isValidTitleId(parsed.titleId)) return {MetadataError::InvalidTitleId, 0};
    output = parsed;
    return {MetadataError::None, 0};
}

} // namespace psbbv::games
