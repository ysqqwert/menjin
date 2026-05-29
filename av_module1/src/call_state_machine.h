#ifndef AV_MODULE_CALL_STATE_MACHINE_H
#define AV_MODULE_CALL_STATE_MACHINE_H

#include <QObject>
#include "../include/av_types.h"

namespace avcall {

/**
 * @brief Finite state machine for call lifecycle.
 *
 * Valid transitions:
 *   Idle     → Calling  (outgoing call)
 *   Idle     → Ringing  (incoming call)
 *   Calling  → Connected (peer accepted)
 *   Calling  → Idle      (peer rejected / timeout / cancel)
 *   Ringing  → Connected (local user accepted)
 *   Ringing  → Idle      (local user rejected / caller cancelled)
 *   Connected→ Idle      (hangup)
 */
class CallStateMachine : public QObject
{
    Q_OBJECT
public:
    explicit CallStateMachine(QObject *parent = nullptr);

    CallState state() const;
    bool tryTransition(CallState to);
    void reset();

signals:
    void stateChanged(avcall::CallState state);

private:
    static bool isValidTransition(CallState from, CallState to);
    CallState m_state = CallState::Idle;
};

} // namespace avcall

#endif // AV_MODULE_CALL_STATE_MACHINE_H
