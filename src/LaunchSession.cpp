#include "psbbv/games/LaunchSession.hpp"
namespace psbbv::games {
LaunchError LaunchSession::beginValidation() noexcept {
    if(state_!=LaunchState::Idle&&state_!=LaunchState::Failed) return LaunchError::InvalidTransition;
    state_=LaunchState::Validating; return LaunchError::None;
}
LaunchError LaunchSession::validationPassed() noexcept {
    if(state_!=LaunchState::Validating) return LaunchError::InvalidTransition;
    state_=LaunchState::Ready; return LaunchError::None;
}
LaunchError LaunchSession::beginLaunch() noexcept {
    if(state_!=LaunchState::Ready) return LaunchError::InvalidTransition;
    state_=LaunchState::Launching; return LaunchError::None;
}
LaunchError LaunchSession::launchSucceeded() noexcept {
    if(state_!=LaunchState::Launching) return LaunchError::InvalidTransition;
    state_=LaunchState::Running; return LaunchError::None;
}
LaunchError LaunchSession::launchFailed() noexcept {
    if(state_!=LaunchState::Launching) return LaunchError::InvalidTransition;
    state_=LaunchState::Failed; return LaunchError::None;
}
LaunchError LaunchSession::requestReturn() noexcept {
    if(state_!=LaunchState::Running) return LaunchError::InvalidTransition;
    state_=LaunchState::Returning; return LaunchError::None;
}
LaunchError LaunchSession::returnedToShell() noexcept {
    if(state_!=LaunchState::Returning) return LaunchError::InvalidTransition;
    state_=LaunchState::Ready; return LaunchError::None;
}
} // namespace psbbv::games
