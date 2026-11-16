#include "psbbv/games/MetadataParser.hpp"

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
using Entry = std::pair<std::string, std::string>;

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

std::vector<std::uint8_t> makeSfo(const std::vector<Entry>& fields) {
    constexpr std::size_t header = 20;
    constexpr std::size_t entrySize = 16;
    std::vector<std::uint8_t> out(header + fields.size() * entrySize, 0);
    out[0] = 0; out[1] = 'P'; out[2] = 'S'; out[3] = 'F';
    write32(out, 4, 0x00000101);
    write32(out, 16, static_cast<std::uint32_t>(fields.size()));

    std::vector<std::uint16_t> keyOffsets;
    for (const auto& field : fields) {
        keyOffsets.push_back(static_cast<std::uint16_t>(out.size() - (header + fields.size() * entrySize)));
        out.insert(out.end(), field.first.begin(), field.first.end());
        out.push_back(0);
    }
    const std::size_t keyStart = header + fields.size() * entrySize;
    const std::size_t dataStart = out.size();
    write32(out, 8, static_cast<std::uint32_t>(keyStart));
    write32(out, 12, static_cast<std::uint32_t>(dataStart));

    std::vector<std::uint32_t> dataOffsets;
    for (const auto& field : fields) {
        dataOffsets.push_back(static_cast<std::uint32_t>(out.size() - dataStart));
        out.insert(out.end(), field.second.begin(), field.second.end());
        out.push_back(0);
    }
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const std::size_t at = header + i * entrySize;
        write16(out, at, keyOffsets[i]);
        write16(out, at + 2, 0x0204);
        const auto length = static_cast<std::uint32_t>(fields[i].second.size() + 1);
        write32(out, at + 4, length);
        write32(out, at + 8, length);
        write32(out, at + 12, dataOffsets[i]);
    }
    return out;
}

std::vector<Entry> validFields() {
    return {{"TITLE_ID", "PCSE00001"}, {"TITLE", "Example Vita Game"},
            {"CATEGORY", "gd"}, {"APP_VER", "01.23"}};
}
}

int main() {
    using namespace psbbv::games;
    GameRecord record{};
    record.favorite = true;
    const auto valid = makeSfo(validFields());
    const MetadataParseResult parsed = parseParamSfo(valid.data(), valid.size(), record);
    require(static_cast<bool>(parsed), "valid Vita PARAM.SFO parses");
    require(std::strcmp(record.titleId, "PCSE00001") == 0, "title ID extracted");
    require(std::strcmp(record.title, "Example Vita Game") == 0, "title extracted");
    require(std::strcmp(record.category, "gd") == 0, "category extracted");
    require(std::strcmp(record.version, "01.23") == 0, "app version extracted");
    require(!record.favorite, "metadata does not import mutable user state");

    GameRecord unchanged{};
    std::strcpy(unchanged.titleId, "PCSE99999");
    auto invalidMagic = valid;
    invalidMagic[0] = 'X';
    require(parseParamSfo(invalidMagic.data(), invalidMagic.size(), unchanged).error ==
            MetadataError::InvalidHeader, "invalid magic rejected");
    require(std::strcmp(unchanged.titleId, "PCSE99999") == 0,
            "failed parse leaves caller output untouched");

    auto badTable = valid;
    write32(badTable, 8, 0xFFFFFFF0);
    require(parseParamSfo(badTable.data(), badTable.size(), unchanged).error ==
            MetadataError::InvalidTable, "out-of-range key table rejected");

    auto truncated = valid;
    write32(truncated, 20 + 12, 0xFFFFFFF0);
    require(parseParamSfo(truncated.data(), truncated.size(), unchanged).error ==
            MetadataError::InvalidValue, "out-of-range value rejected");

    auto oversizedCapacity = valid;
    write32(oversizedCapacity, 20 + 8, 0xFFFFFFF0);
    require(parseParamSfo(oversizedCapacity.data(), oversizedCapacity.size(), unchanged).error ==
            MetadataError::InvalidValue, "out-of-range reserved value capacity rejected");

    auto unsupportedVersion = valid;
    write32(unsupportedVersion, 4, 0x00000201);
    require(parseParamSfo(unsupportedVersion.data(), unsupportedVersion.size(), unchanged).error ==
            MetadataError::UnsupportedVersion, "unsupported PSF version rejected");

    std::vector<Entry> tooMany(129, {"IGNORED", "x"});
    const auto excessiveEntries = makeSfo(tooMany);
    require(parseParamSfo(excessiveEntries.data(), excessiveEntries.size(), unchanged).error ==
            MetadataError::TooManyEntries, "excessive entry count rejected");

    auto duplicate = validFields();
    duplicate.push_back({"TITLE_ID", "PCSE00002"});
    const auto duplicateSfo = makeSfo(duplicate);
    require(parseParamSfo(duplicateSfo.data(), duplicateSfo.size(), unchanged).error ==
            MetadataError::DuplicateField, "duplicate required metadata rejected");

    auto missingTitle = makeSfo({{"TITLE_ID", "PCSE00001"}});
    require(parseParamSfo(missingTitle.data(), missingTitle.size(), unchanged).error ==
            MetadataError::MissingTitle, "missing title rejected");

    auto invalidId = makeSfo({{"TITLE_ID", "../bad-id"}, {"TITLE", "Bad"}});
    require(parseParamSfo(invalidId.data(), invalidId.size(), unchanged).error ==
            MetadataError::InvalidTitleId, "path-like title ID rejected");

    auto oversized = makeSfo({{"TITLE_ID", "PCSE00001"}, {"TITLE", std::string(TitleCapacity, 'T')}});
    require(parseParamSfo(oversized.data(), oversized.size(), unchanged).error ==
            MetadataError::FieldTooLong, "oversized title rejected");

    require(parseParamSfo(nullptr, 0, unchanged).error == MetadataError::InvalidArgument,
            "null input rejected");
    std::cout << "PSBBV Games PARAM.SFO parser: all checks passed.\n";
}
