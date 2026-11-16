#pragma once

#include "psbbv/games/Discovery.hpp"

namespace psbbv::games::vita {

// Owns only the open directory descriptor for the fixed ux0:/app root.
struct VitaDiscoveryContext final {
    int directoryFd = -1;
};

// Returns a user-mode VitaSDK provider. No privileged Shell APIs are used.
DiscoverySource makeVitaDiscoverySource(VitaDiscoveryContext& context) noexcept;

} // namespace psbbv::games::vita
