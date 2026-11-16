#include "psbbv/games/VitaDiscovery.hpp"

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include <cstdio>
#include <cstring>

namespace psbbv::games::vita {
namespace {

constexpr char ApplicationRoot[] = "ux0:/app";
constexpr char ParamSfoName[] = "param.sfo";
constexpr char SystemDirectoryName[] = "sce_sys";

bool isSymlink(const SceIoStat& stat) noexcept {
    return (stat.st_mode & SCE_S_IFMT) == SCE_S_IFLNK ||
           (stat.st_attr & SCE_SO_IFMT) == SCE_SO_IFLNK;
}

bool isDirectory(const SceIoStat& stat) noexcept {
    return (stat.st_mode & SCE_S_IFMT) == SCE_S_IFDIR ||
           (stat.st_attr & SCE_SO_IFMT) == SCE_SO_IFDIR;
}

bool isRegularFile(const SceIoStat& stat) noexcept {
    return (stat.st_mode & SCE_S_IFMT) == SCE_S_IFREG ||
           (stat.st_attr & SCE_SO_IFMT) == SCE_SO_IFREG;
}

bool entryNameEquals(const SceIoDirent& entry, const char* expected) noexcept {
    const void* terminator = std::memchr(entry.d_name, '\0', sizeof(entry.d_name));
    if (terminator == nullptr) return false;
    const std::size_t length = static_cast<const char*>(terminator) - entry.d_name;
    const std::size_t expectedLength = std::strlen(expected);
    return length == expectedLength &&
           std::memcmp(entry.d_name, expected, expectedLength) == 0;
}

int openRoot(void* opaque) noexcept {
    auto& context = *static_cast<VitaDiscoveryContext*>(opaque);
    if (context.directoryFd >= 0) return -1;
    const SceUID fd = sceIoDopen(ApplicationRoot);
    if (fd < 0) return -1;
    context.directoryFd = fd;
    return 0;
}

int nextRootEntry(void* opaque, char* name, std::size_t capacity,
                  std::size_t* nameLength, bool* directory, bool* symlink) noexcept {
    auto& context = *static_cast<VitaDiscoveryContext*>(opaque);
    if (context.directoryFd < 0 || name == nullptr || capacity == 0 ||
        nameLength == nullptr || directory == nullptr || symlink == nullptr) return -1;

    SceIoDirent entry{};
    const int result = sceIoDread(context.directoryFd, &entry);
    if (result <= 0) return result;

    const void* terminator = std::memchr(entry.d_name, '\0', sizeof(entry.d_name));
    if (terminator == nullptr) {
        *nameLength = sizeof(entry.d_name);
        name[0] = '\0';
        *directory = false;
        *symlink = false;
        return 1;
    }

    const std::size_t length = static_cast<const char*>(terminator) - entry.d_name;
    *nameLength = length;
    const std::size_t copyLength = length < capacity - 1 ? length : capacity - 1;
    std::memcpy(name, entry.d_name, copyLength);
    name[copyLength] = '\0';
    *directory = isDirectory(entry.d_stat);
    *symlink = isSymlink(entry.d_stat);
    return 1;
}

bool findSafeDirectory(const char* path, const char* wanted) noexcept {
    const SceUID fd = sceIoDopen(path);
    if (fd < 0) return false;
    bool found = false;
    for (;;) {
        SceIoDirent entry{};
        const int result = sceIoDread(fd, &entry);
        if (result <= 0) break;
        if (entryNameEquals(entry, wanted)) {
            found = isDirectory(entry.d_stat) && !isSymlink(entry.d_stat);
            break;
        }
    }
    sceIoDclose(fd);
    return found;
}

bool findSafeMetadata(const char* path, SceOff& size) noexcept {
    const SceUID fd = sceIoDopen(path);
    if (fd < 0) return false;
    bool found = false;
    for (;;) {
        SceIoDirent entry{};
        const int result = sceIoDread(fd, &entry);
        if (result <= 0) break;
        if (entryNameEquals(entry, ParamSfoName)) {
            if (isRegularFile(entry.d_stat) && !isSymlink(entry.d_stat) &&
                entry.d_stat.st_size > 0 &&
                entry.d_stat.st_size <= static_cast<SceOff>(MaxParamSfoBytes)) {
                size = entry.d_stat.st_size;
                found = true;
            }
            break;
        }
    }
    sceIoDclose(fd);
    return found;
}

int readMetadata(void* opaque, const char* titleId, std::uint8_t* output,
                 std::size_t capacity, std::size_t* bytesRead,
                 bool* regular, bool* symlink) noexcept {
    (void)opaque;
    if (!isValidTitleId(titleId) || output == nullptr || bytesRead == nullptr ||
        regular == nullptr || symlink == nullptr || capacity == 0 ||
        capacity > MaxParamSfoBytes) return -1;

    char titlePath[64]{};
    char systemPath[80]{};
    char metadataPath[112]{};
    const int titleLength = std::snprintf(titlePath, sizeof(titlePath),
                                          "%s/%s", ApplicationRoot, titleId);
    const int systemLength = std::snprintf(systemPath, sizeof(systemPath),
                                           "%s/%s", titlePath, SystemDirectoryName);
    const int metadataLength = std::snprintf(metadataPath, sizeof(metadataPath),
                                             "%s/%s", systemPath, ParamSfoName);
    if (titleLength <= 0 || static_cast<std::size_t>(titleLength) >= sizeof(titlePath) ||
        systemLength <= 0 || static_cast<std::size_t>(systemLength) >= sizeof(systemPath) ||
        metadataLength <= 0 || static_cast<std::size_t>(metadataLength) >= sizeof(metadataPath))
        return -1;

    if (!findSafeDirectory(titlePath, SystemDirectoryName)) return -1;

    SceOff listedSize = 0;
    if (!findSafeMetadata(systemPath, listedSize) ||
        listedSize > static_cast<SceOff>(capacity)) return -1;

    const SceUID fd = sceIoOpen(metadataPath, SCE_O_RDONLY, 0);
    if (fd < 0) return -1;

    SceIoStat openedStat{};
    bool success = sceIoGetstatByFd(fd, &openedStat) >= 0 &&
                   isRegularFile(openedStat) && !isSymlink(openedStat) &&
                   openedStat.st_size > 0 &&
                   openedStat.st_size <= static_cast<SceOff>(capacity) &&
                   openedStat.st_size <= static_cast<SceOff>(MaxParamSfoBytes);

    std::size_t total = 0;
    if (success) {
        const std::size_t expected = static_cast<std::size_t>(openedStat.st_size);
        while (total < expected) {
            const SceSSize amount = sceIoRead(fd, output + total, expected - total);
            if (amount <= 0) {
                success = false;
                break;
            }
            total += static_cast<std::size_t>(amount);
        }
        success = success && total == expected &&
                  openedStat.st_size == listedSize;
    }

    sceIoClose(fd);
    if (!success) return -1;
    *bytesRead = total;
    *regular = true;
    *symlink = false;
    return 0;
}

void closeRoot(void* opaque) noexcept {
    auto& context = *static_cast<VitaDiscoveryContext*>(opaque);
    if (context.directoryFd >= 0) {
        sceIoDclose(context.directoryFd);
        context.directoryFd = -1;
    }
}

} // namespace

DiscoverySource makeVitaDiscoverySource(VitaDiscoveryContext& context) noexcept {
    return {&context, openRoot, nextRootEntry, readMetadata, closeRoot};
}

} // namespace psbbv::games::vita
