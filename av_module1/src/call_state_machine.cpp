#include "call_state_machine.h"

namespace avcall {

CallStateMachine::CallStateMachine(QObject *parent)
    : QObject(parent)
{
}

CallState CallStateMachine::state() const
{
    return m_state;
}

bool CallStateMachine::tryTransition(CallState to)
{
    if (!isValidTransition(m_state, to))
        return false;

    m_state = to;
    emit stateChanged(m_state);
    return true;
}

void CallStateMachine::reset()
{
    if (m_state != CallState::Idle) {
        m_state = CallState::Idle;
        emit stateChanged(m_state);
    }
}

bool CallStateMachine::isValidTransition(CallState from, CallState to)
{
    switch (from) {
    case CallState::Idle:
        return to == CallState::Calling || to == CallState::Ringing;
    case CallState::Calling:
        return to == CallState::Connected || to == CallState::Idle;
    case CallState::Ringing:
        return to == CallState::Connected || to == CallState::Idle;
    case CallState::Connected:
        return to == CallState::Idle;
    }
    return false;
}

} // namespace avcall
