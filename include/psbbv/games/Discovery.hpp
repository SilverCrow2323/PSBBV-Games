#pragma once

#include "psbbv/games/MetadataParser.hpp"

#include <cstddef>
#include <cstdint>

namespace psbbv::games {

constexpr std::size_t MaxParamSfoBytes = 16 * 1024;
constexpr std::size_t MaxDiscoveryDirectoryEntries = 512;
constexpr std::size_t DiscoveryNameCapacity = 64;

enum class DiscoveryError : std::uint8_t {
    None,
    InvalidArgument,
    SourceOpenFailed,
    SourceEnumerationFailed,
    TooManyDirectoryEntries,
    TooManyTitles,
    CatalogueRejected
};

struct DiscoverySource final {
    void* context = nullptr;
    int (*open)(void*) noexcept = nullptr;
    // next: 1 = entry, 0 = end, negative = enumeration error.
    // nameLength is the byte count excluding the required NUL terminator.
    int (*next)(void*, char* name, std::size_t capacity, std::size_t* nameLength,
                bool* isDirectory, bool* isSymlink) noexcept = nullptr;
    // readParamSfo: zero = read completed; nonzero = this title could not be read.
    // The adapter must stay inside its configured root, stat before reading,
    // reject non-regular/symlink files and never write beyond output capacity.
    int (*readParamSfo)(void*, const char* titleId, std::uint8_t* output,
                        std::size_t capacity, std::size_t* bytesRead,
                        bool* isRegularFile, bool* isSymlink) noexcept = nullptr;
    void (*close)(void*) noexcept = nullptr;
};

struct DiscoveryReport final {
    DiscoveryError error = DiscoveryError::None;
    CatalogueError catalogueError = CatalogueError::None;
    std::size_t directoryEntries = 0;
    std::size_t acceptedRecords = 0;
    std::size_t skippedEntries = 0;
    std::size_t unreadableMetadata = 0;
    std::size_t malformedMetadata = 0;
    std::size_t removedStaleRecords = 0;
};

// Performs a transactional, bounded scan through an injected read-only source.
// The caller supplies the PARAM.SFO scratch buffer; no filesystem API is used here.
DiscoveryReport discoverInstalledTitles(Catalogue& catalogue,
                                        const DiscoverySource& source,
                                        std::uint8_t* scratch,
                                        std::size_t scratchCapacity) noexcept;

} // namespace psbbv::games
