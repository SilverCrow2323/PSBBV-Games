#include "psbbv/games/LaunchSession.hpp"
#include <cassert>
#include <iostream>
using namespace psbbv::games;
int main() {
    LaunchSession session;
    assert(session.beginLaunch()==LaunchError::InvalidTransition);
    assert(session.beginValidation()==LaunchError::None);
    assert(session.validationPassed()==LaunchError::None);
    assert(session.beginLaunch()==LaunchError::None);
    assert(session.requestReturn()==LaunchError::InvalidTransition);
    assert(session.launchSucceeded()==LaunchError::None);
    assert(session.requestReturn()==LaunchError::None);
    assert(session.returnedToShell()==LaunchError::None);
    assert(session.state()==LaunchState::Ready);
    assert(session.beginLaunch()==LaunchError::None);
    assert(session.launchFailed()==LaunchError::None);
    assert(session.beginValidation()==LaunchError::None);
    std::cout << "PSBBV game launch lifecycle tests passed\n";
}
