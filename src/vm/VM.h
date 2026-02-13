#ifndef PETUKH_VM_H
#define PETUKH_VM_H

#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include "../rpn/RPNInstruction.h"
#include "Value.h"



class VM {
 public:
  explicit VM(const std::vector<Instruction> &code);
  // Run the program. Returns exit code (0 if normal return from main).
  int Run();

 private:
  const std::vector<Instruction> &code_;

  struct Frame {
    size_t ret_ip = 0;
    std::map<std::string, Value> locals;
  };

  std::vector<Value> stack_;
  std::vector<Frame> call_stack_;
  std::map<std::string, size_t> label_map_;

  void BuildLabelMap();

  // stack helpers
  void Push(const Value &v);
  Value Pop();
  Value Peek(size_t depth = 0) const;  // 0 = top

  // instruction handlers
  void HandlePushInt(const std::string &arg);
  void HandlePushDouble(const std::string &arg);
  void HandlePushString(const std::string &arg);

  void HandleLoad(const std::string &name);
  void HandleStore(const std::string &name);
  void HandleNewArray();
  void HandleLoadIndex();
  void HandleStoreIndex(const std::string &varName);

  void HandleBinaryOp(OpCode op);
  void HandleUnaryOp(OpCode op);

  bool IsBuiltin(const std::string &name) const;
  void CallBuiltin(const std::string &name);
};

#endif  // PETUKH_VM_H
