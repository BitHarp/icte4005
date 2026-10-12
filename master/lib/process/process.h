#pragma once
#include <stdint.h>
#include "contract.h"
#include "feedback.h"

enum MasterState {
  MASTER_INITIALISING = 0,
  MASTER_READY,
  MASTER_FORWARD,
  MASTER_TRACKING,
  MASTER_STOPPING,
  MASTER_WAIT_ACK,
  MASTER_RETURNING,
  MASTER_OVERRUN,
  MASTER_CLEARING,
  MASTER_PAUSED,
  MASTER_BENCH,
  MASTER_FAULT
};

enum MasterFault {
  MASTER_FAULT_NONE = 0,
  MASTER_FAULT_MISSING_FORWARD,
  MASTER_FAULT_MISSING_RETURN,
  MASTER_FAULT_REED_ORDER,
  MASTER_FAULT_FEEDBACK_LOST,
  MASTER_FAULT_FEEDBACK_STALE,
  MASTER_FAULT_INVALID_REPLY,
  MASTER_FAULT_SLAVE_FAULT,
  MASTER_FAULT_UNEXPECTED_MOTION,
  MASTER_FAULT_CONFIRM_TIMEOUT,
  MASTER_FAULT_STOP_UNCONFIRMED,
  MASTER_FAULT_SLAVE_REBOOT,
  MASTER_FAULT_REED_HELD
};

enum OperatorKind {
  OPERATOR_NONE = 0,
  OPERATOR_START,
  OPERATOR_STOP,
  OPERATOR_RESUME,
  OPERATOR_ACK,
  OPERATOR_BENCH,
  OPERATOR_SEND
};

struct OperatorCommand {
  OperatorKind kind;
  uint8_t address;
  char command;
};

enum Notice {
  NOTICE_NONE = 0,
  NOTICE_START_NOT_CALIBRATED,
  NOTICE_START_NOT_READY,
  NOTICE_START_REED_ACTIVE,
  NOTICE_ACK_REED1_ACTIVE,
  NOTICE_IGNORED,
  NOTICE_INIT_WAITING,
  NOTICE_INIT_REEDS_ACTIVE,
  NOTICE_BENCH_ENTERED,
  NOTICE_RAW_SENT,
  NOTICE_STOP_ALREADY,
  NOTICE_RESUMED
};

enum AndonColour { ANDON_OFF, ANDON_GREEN, ANDON_AMBER, ANDON_RED };

struct Calibration {
  uint32_t forwardAllowanceSteps;
  uint32_t returnAllowanceSteps;
  uint32_t dischargeOverrunSteps;
  bool ready() const {
    return forwardAllowanceSteps > 0 && returnAllowanceSteps > 0 && dischargeOverrunSteps > 0;
  }
};

struct Request {
  uint8_t address;
  char command;
};

struct ProcessOutput {
  Request requests[4];
  uint8_t requestCount;
  Notice notice;
  void clear() { requestCount = 0; notice = NOTICE_NONE; }
  void add(uint8_t address, char command) {
    if (requestCount < 4) {
      requests[requestCount].address = address;
      requests[requestCount].command = command;
      requestCount++;
    }
  }
};

// Everything a state handler may know about the slaves. Computed once per tick.
struct Observation {
  bool conveyorFresh;
  bool sensingFresh;
  bool conveyorStopped;
  bool conveyorForward;
  bool conveyorReverse;
  bool reedsClear;
  bool reed1Closed;
  bool arrived;
  bool magnetic;
  bool reedOrderBad;
  bool returned;
  bool forwardOverdue;
  bool returnOverdue;
  bool overrunDone;
};

const char* masterStateName(MasterState state);
const char* masterFaultName(MasterFault fault);

class Process {
 public:
  void begin(const Calibration& calibration);
  void tick(uint32_t now, SlaveView& conveyor, SlaveView& sensing, const OperatorCommand& operatorCommand,
            ProcessOutput& output);

  MasterState state() const { return state_; }
  MasterFault fault() const { return fault_; }
  MasterState savedPhase() const { return savedPhase_; }
  bool stopUnconfirmed() const { return stopUnconfirmed_; }
  bool calibrated() const { return calibration_.ready(); }
  uint8_t conveyorTarget() const;
  AndonColour andon() const;
  bool travel(uint32_t steps, uint32_t& delta, uint32_t& limit) const;

 private:
  enum ConfirmResult { CONFIRM_IDLE, CONFIRM_WAITING, CONFIRM_DONE, CONFIRM_TIMED_OUT };
  struct Pending {
    bool active;
    char command;
    uint8_t expectedState;
    bool retryEachPoll;
    uint32_t sentMs;
    uint32_t deadline;
  };

  MasterState activePhase() const;
  bool inForwardPhase() const;
  bool inReturnPhase() const;
  bool inCycle() const;

  void sendMotion(ProcessOutput& output, char command, uint8_t expectedState, bool retryEachPoll, uint32_t now);
  ConfirmResult confirm(const SlaveView& conveyor, ProcessOutput& output, uint32_t now);
  void enterFault(MasterFault reason, ProcessOutput& output, uint32_t now);
  void enterInitialising();
  void enterStopping(MasterState saved, ProcessOutput& output, uint32_t now);
  void resetCycle();

  bool checkFeedback(SlaveView& conveyor, SlaveView& sensing, ProcessOutput& output, uint32_t now);
  void updateBaselines(const SlaveView& conveyor, const SlaveView& sensing);
  Observation observe(const SlaveView& conveyor, const SlaveView& sensing) const;
  bool checkProgress(const Observation& seen, ProcessOutput& output, uint32_t now);
  void startForward(const Observation& seen, ProcessOutput& output, uint32_t now);
  void startReverse(const SlaveView& conveyor, const SlaveView& sensing, const Observation& seen,
                    ProcessOutput& output, uint32_t now);
  void finishStop(const Observation& seen, ProcessOutput& output, uint32_t now);
  void resume(const SlaveView& conveyor, const SlaveView& sensing, const Observation& seen, ProcessOutput& output,
              uint32_t now);
  void tickClearing(const Observation& seen, ProcessOutput& output, uint32_t now);

  MasterState state_;
  MasterState savedPhase_;
  MasterFault fault_;
  Calibration calibration_;
  Pending pending_;
  bool initialStopRequested_;
  bool stopUnconfirmed_;

  bool hasReedBaseline_;
  uint16_t reed1Start_;
  uint16_t reed2Start_;
  bool hasEntryBaseline_;
  uint32_t entryBaseline_;
  bool hasReturnBaseline_;
  uint16_t reed1ReturnStart_;
  uint32_t returnBaseline_;
  bool hasOverrunBaseline_;
  uint32_t overrunBaseline_;
  bool clearingWaitStarted_;
  uint32_t clearingWaitStartMs_;
};
