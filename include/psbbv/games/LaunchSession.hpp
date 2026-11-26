#pragma once
#include <cstdint>
namespace psbbv::games {
enum class LaunchState : std::uint8_t { Idle, Validating, Ready, Launching, Running, Returning, Failed };
enum class LaunchError : std::uint8_t { None, InvalidTransition };
class LaunchSession final {
public:
    LaunchError beginValidation() noexcept;
    LaunchError validationPassed() noexcept;
    LaunchError beginLaunch() noexcept;
    LaunchError launchSucceeded() noexcept;
    LaunchError launchFailed() noexcept;
    LaunchError requestReturn() noexcept;
    LaunchError returnedToShell() noexcept;
    LaunchState state() const noexcept { return state_; }
private:
    LaunchState state_ = LaunchState::Idle;
};
} // namespace psbbv::games
