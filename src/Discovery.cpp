#include "psbbv/games/Discovery.hpp"

#include <cstring>

namespace psbbv::games {
namespace {

bool wasSeen(const char seen[MaxGames][TitleIdCapacity], std::size_t count,
             const char* titleId) noexcept {
    for (std::size_t i = 0; i < count; ++i)
        if (std::strcmp(seen[i], titleId) == 0) return true;
    return false;
}

} // namespace

DiscoveryReport discoverInstalledTitles(Catalogue& catalogue,
                                        const DiscoverySource& source,
                                        std::uint8_t* scratch,
                                        std::size_t scratchCapacity) noexcept {
    DiscoveryReport report{};
    if (source.open == nullptr || source.next == nullptr ||
        source.readParamSfo == nullptr || source.close == nullptr ||
        scratch == nullptr || scratchCapacity == 0 ||
        scratchCapacity > MaxParamSfoBytes) {
        report.error = DiscoveryError::InvalidArgument;
        return report;
    }
    if (source.open(source.context) != 0) {
        report.error = DiscoveryError::SourceOpenFailed;
        return report;
    }

    Catalogue staged = catalogue;
    char seen[MaxGames][TitleIdCapacity]{};
    std::size_t seenCount = 0;
    bool enumerationFailed = false;

    for (;;) {
        char name[DiscoveryNameCapacity]{};
        std::size_t nameLength = 0;
        bool isDirectory = false;
        bool isSymlink = false;
        const int nextResult = source.next(source.context, name, sizeof(name),
                                           &nameLength, &isDirectory, &isSymlink);
        if (nextResult == 0) break;
        if (nextResult < 0) {
            enumerationFailed = true;
            report.error = DiscoveryError::SourceEnumerationFailed;
            break;
        }
        if (++report.directoryEntries > MaxDiscoveryDirectoryEntries) {
            report.error = DiscoveryError::TooManyDirectoryEntries;
            break;
        }

        if (nameLength != 9 || nameLength >= sizeof(name) ||
            name[nameLength] != '\0' || !isDirectory || isSymlink ||
            !isValidTitleId(name)) {
            ++report.skippedEntries;
            continue;
        }

        if (!wasSeen(seen, seenCount, name)) {
            if (seenCount == MaxGames) {
                report.error = DiscoveryError::TooManyTitles;
                break;
            }
            std::memcpy(seen[seenCount], name, TitleIdCapacity);
            ++seenCount;
        }

        std::size_t bytesRead = 0;
        bool isRegularFile = false;
        bool metadataSymlink = false;
        const int readResult = source.readParamSfo(source.context, name, scratch,
                                                    scratchCapacity, &bytesRead,
                                                    &isRegularFile, &metadataSymlink);
        if (readResult != 0) {
            ++report.unreadableMetadata;
            continue;
        }
        if (!isRegularFile || metadataSymlink || bytesRead == 0 ||
            bytesRead > scratchCapacity || bytesRead > MaxParamSfoBytes) {
            ++report.malformedMetadata;
            continue;
        }

        GameRecord parsed{};
        const MetadataParseResult parse = parseParamSfo(scratch, bytesRead, parsed);
        if (!parse) {
            ++report.malformedMetadata;
            continue;
        }
        if (std::strcmp(parsed.titleId, name) != 0) {
            ++report.malformedMetadata;
            continue;
        }

        // Package metadata must never reset user-owned catalogue state.
        if (const GameRecord* existing = staged.find(parsed.titleId)) {
            parsed.favorite = existing->favorite;
            parsed.lastPlayedUnix = existing->lastPlayedUnix;
        }

        report.catalogueError = staged.upsert(parsed);
        if (report.catalogueError != CatalogueError::None) {
            report.error = DiscoveryError::CatalogueRejected;
            break;
        }
        ++report.acceptedRecords;
    }

    source.close(source.context);
    if (report.error != DiscoveryError::None || enumerationFailed) return report;

    for (std::size_t i = 0; i < staged.size();) {
        const GameRecord* record = staged.at(i);
        if (record != nullptr && !wasSeen(seen, seenCount, record->titleId)) {
            char staleId[TitleIdCapacity]{};
            std::memcpy(staleId, record->titleId, sizeof(staleId));
            if (staged.remove(staleId)) ++report.removedStaleRecords;
        } else {
            ++i;
        }
    }

    catalogue = staged;
    return report;
}

} // namespace psbbv::games
