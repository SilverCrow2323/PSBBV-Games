#include "psbbv/games/Discovery.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace psbbv::games;

void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
void write16(std::vector<std::uint8_t>& out, std::size_t at, std::uint16_t value) {
    out[at] = static_cast<std::uint8_t>(value);
    out[at + 1] = static_cast<std::uint8_t>(value >> 8);
}
void write32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) out[at + i] = static_cast<std::uint8_t>(value >> (i * 8));
}
std::vector<std::uint8_t> makeSfo(const char* id, const char* title) {
    const std::vector<std::pair<std::string, std::string>> fields{
        {"TITLE_ID", id}, {"TITLE", title}, {"CATEGORY", "gd"}, {"APP_VER", "01.00"}};
    constexpr std::size_t header = 20;
    constexpr std::size_t entrySize = 16;
    std::vector<std::uint8_t> out(header + fields.size() * entrySize, 0);
    out[0] = 0; out[1] = 'P'; out[2] = 'S'; out[3] = 'F';
    write32(out, 4, 0x00000101);
    write32(out, 16, static_cast<std::uint32_t>(fields.size()));
    std::vector<std::uint16_t> keys;
    for (const auto& field : fields) {
        keys.push_back(static_cast<std::uint16_t>(out.size() - (header + fields.size() * entrySize)));
        out.insert(out.end(), field.first.begin(), field.first.end());
        out.push_back(0);
    }
    const std::size_t keyStart = header + fields.size() * entrySize;
    const std::size_t dataStart = out.size();
    write32(out, 8, static_cast<std::uint32_t>(keyStart));
    write32(out, 12, static_cast<std::uint32_t>(dataStart));
    std::vector<std::uint32_t> values;
    for (const auto& field : fields) {
        values.push_back(static_cast<std::uint32_t>(out.size() - dataStart));
        out.insert(out.end(), field.second.begin(), field.second.end());
        out.push_back(0);
    }
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const std::size_t at = header + i * entrySize;
        write16(out, at, keys[i]);
        write16(out, at + 2, 0x0204);
        const auto length = static_cast<std::uint32_t>(fields[i].second.size() + 1);
        write32(out, at + 4, length);
        write32(out, at + 8, length);
        write32(out, at + 12, values[i]);
    }
    return out;
}
GameRecord record(const char* id, const char* title, bool favorite = false) {
    GameRecord value{};
    std::strncpy(value.titleId, id, sizeof(value.titleId) - 1);
    std::strncpy(value.title, title, sizeof(value.title) - 1);
    value.favorite = favorite;
    return value;
}
struct Item {
    std::string name;
    bool directory = true;
    bool directorySymlink = false;
    bool regularFile = true;
    bool fileSymlink = false;
    bool readFailure = false;
    std::vector<std::uint8_t> bytes;
};
struct FakeSource {
    std::vector<Item> items;
    std::size_t cursor = 0;
    std::size_t failAt = static_cast<std::size_t>(-1);
    bool opened = false;
    bool closed = false;
};
int openSource(void* context) noexcept {
    auto& source = *static_cast<FakeSource*>(context);
    source.cursor = 0;
    source.opened = true;
    source.closed = false;
    return 0;
}
int nextSource(void* context, char* name, std::size_t capacity, std::size_t* length,
               bool* directory, bool* symlink) noexcept {
    auto& source = *static_cast<FakeSource*>(context);
    if (source.cursor == source.failAt) return -1;
    if (source.cursor >= source.items.size()) return 0;
    const Item& item = source.items[source.cursor++];
    *length = item.name.size();
    const std::size_t copyLength = std::min(item.name.size(), capacity - 1);
    std::memcpy(name, item.name.data(), copyLength);
    name[copyLength] = '\0';
    *directory = item.directory;
    *symlink = item.directorySymlink;
    return 1;
}
int readSource(void* context, const char* titleId, std::uint8_t* output,
               std::size_t capacity, std::size_t* bytesRead,
               bool* regular, bool* symlink) noexcept {
    auto& source = *static_cast<FakeSource*>(context);
    for (const Item& item : source.items) {
        if (item.name == titleId) {
            if (item.readFailure) return -1;
            *regular = item.regularFile;
            *symlink = item.fileSymlink;
            *bytesRead = item.bytes.size();
            if (item.bytes.size() > capacity) return 0;
            std::memcpy(output, item.bytes.data(), item.bytes.size());
            return 0;
        }
    }
    return -1;
}
void closeSource(void* context) noexcept {
    static_cast<FakeSource*>(context)->closed = true;
}
DiscoverySource adapter(FakeSource& source) {
    return {&source, openSource, nextSource, readSource, closeSource};
}
}

int main() {
    Catalogue catalogue;
    GameRecord existingFavorite = record("PCSE00001", "Old title", true);
    existingFavorite.lastPlayedUnix = 123456;
    require(catalogue.upsert(existingFavorite) == CatalogueError::None,
            "existing favourite and play history inserted");
    require(catalogue.upsert(record("PCSE00002", "Stale title")) == CatalogueError::None,
            "stale record inserted");

    FakeSource fake;
    fake.items.push_back({"PCSE00001", true, false, true, false, false,
                          makeSfo("PCSE00001", "Updated title")});
    fake.items.push_back({"PCSE00003", true, false, true, false, false,
                          makeSfo("PCSE00003", "New title")});
    fake.items.push_back({"../bad-id", true, false, true, false, false, {}});
    fake.items.push_back({"PCSE00004", true, true, true, false, false, {}});
    fake.items.push_back({"PCSE00005", true, false, true, true, false,
                          makeSfo("PCSE00005", "Symlinked metadata")});
    fake.items.push_back({"PCSE00006", true, false, true, false, false, {0x00, 'P', 'S', 'F'}});
    fake.items.push_back({"PCSE00007", true, false, true, false, true, {}});
    std::uint8_t scratch[MaxParamSfoBytes]{};
    const DiscoveryReport report = discoverInstalledTitles(catalogue, adapter(fake),
                                                            scratch, sizeof(scratch));
    require(report.error == DiscoveryError::None, "complete scan succeeds");
    require(fake.opened && fake.closed, "source is closed after scan");
    require(report.acceptedRecords == 2, "two valid metadata records accepted");
    require(report.skippedEntries == 2, "invalid and symlink directories skipped");
    require(report.malformedMetadata == 2, "symlink and malformed metadata rejected");
    require(report.unreadableMetadata == 1, "read error counted without aborting scan");
    require(report.removedStaleRecords == 1, "stale record removed after complete scan");
    require(catalogue.size() == 2, "catalogue contains only present valid titles");
    require(std::strcmp(catalogue.find("PCSE00001")->title, "Updated title") == 0,
            "metadata refresh updates title");
    require(catalogue.find("PCSE00001")->favorite, "existing user favourite is preserved");
    require(catalogue.find("PCSE00001")->lastPlayedUnix == 123456,
            "existing last-played timestamp is preserved");
    require(catalogue.find("PCSE00003") != nullptr, "new installed title added");
    require(catalogue.find("PCSE00002") == nullptr, "stale title removed");

    Catalogue unchanged;
    unchanged.upsert(record("PCSE00009", "Keep me"));
    FakeSource interrupted;
    interrupted.items.push_back({"PCSE00010", true, false, true, false, false,
                                 makeSfo("PCSE00010", "Partial")});
    interrupted.failAt = 1;
    const DiscoveryReport failed = discoverInstalledTitles(unchanged, adapter(interrupted),
                                                             scratch, sizeof(scratch));
    require(failed.error == DiscoveryError::SourceEnumerationFailed,
            "enumeration error reported");
    require(interrupted.closed, "source closed after enumeration error");
    require(unchanged.size() == 1 && unchanged.find("PCSE00009") != nullptr,
            "failed scan leaves original catalogue untouched");

    require(discoverInstalledTitles(unchanged, {}, scratch, sizeof(scratch)).error ==
            DiscoveryError::InvalidArgument, "missing source callbacks rejected");
    FakeSource oversized;
    for (std::size_t i = 0; i < MaxDiscoveryDirectoryEntries + 1; ++i)
        oversized.items.push_back({"not-a-title", true, false, true, false, false, {}});
    const DiscoveryReport overflow = discoverInstalledTitles(unchanged, adapter(oversized),
                                                               scratch, sizeof(scratch));
    require(overflow.error == DiscoveryError::TooManyDirectoryEntries,
            "oversized directory is rejected at the configured bound");
    require(overflow.directoryEntries == MaxDiscoveryDirectoryEntries + 1,
            "directory entry count reports the first rejected entry");
    require(oversized.closed, "source closes after entry-count overflow");
    require(unchanged.size() == 1 && unchanged.find("PCSE00009") != nullptr,
            "overflow scan leaves original catalogue untouched");
    std::cout << "PSBBV Games installed-title discovery: all checks passed.\n";
}
