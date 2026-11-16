#pragma once

#include "psbbv/games/Catalogue.hpp"

#include <cstddef>
#include <cstdint>

namespace psbbv::games {

enum class MetadataError : std::uint8_t {
    None,
    InvalidArgument,
    InvalidHeader,
    UnsupportedVersion,
    InvalidTable,
    TooManyEntries,
    InvalidKey,
    InvalidValue,
    DuplicateField,
    MissingTitleId,
    MissingTitle,
    InvalidTitleId,
    FieldTooLong
};

struct MetadataParseResult final {
    MetadataError error = MetadataError::None;
    std::size_t entryIndex = 0;

    explicit operator bool() const noexcept { return error == MetadataError::None; }
};

// Parses the bounded, standard PSF metadata used by installed Vita titles'
// sce_sys/param.sfo. It never opens paths or allocates memory.
MetadataParseResult parseParamSfo(const std::uint8_t* bytes,
                                  std::size_t size,
                                  GameRecord& output) noexcept;

} // namespace psbbv::games
