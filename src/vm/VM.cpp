#include "VM.h"
#include <iostream>
#include <sstream>
#include <stdexcept>



VM::VM(const std::vector<Instruction> &code) : code_(code) {
  BuildLabelMap();
}

void VM::BuildLabelMap() {
  label_map_.clear();
  for (size_t i = 0; i < code_.size(); ++i) {
    if (code_[i].op == OpCode::LABEL) {
      // map label name -> instruction index of the LABEL (execution will skip it)
      label_map_[code_[i].arg] = i;
    }
  }
}

int VM::Run() {
  // find "main" label and start there if present
  size_t ip = 0;
  if (label_map_.count("main")) {
    ip = label_map_["main"];
    // push a bottom frame with ret_ip == end (terminates on RET when popped)
    Frame f; f.ret_ip = code_.size(); call_stack_.push_back(std::move(f));
    // set ip to the label so execution begins at label (LABEL will be skipped)
  }

  while (ip < code_.size()) {
    const Instruction &ins = code_[ip];

    switch (ins.op) {
      case OpCode::POP:
        Pop();
        ip++;
        break;

      case OpCode::PUSH_INT: HandlePushInt(ins.arg); ip++; break;
      case OpCode::PUSH_DOUBLE: HandlePushDouble(ins.arg); ip++; break;
      case OpCode::PUSH_STRING: HandlePushString(ins.arg); ip++; break;

      case OpCode::LOAD: HandleLoad(ins.arg); ip++; break;
      case OpCode::STORE: HandleStore(ins.arg); ip++; break;

      case OpCode::NEW_ARRAY: HandleNewArray(); ip++; break;
      case OpCode::LOAD_INDEX: HandleLoadIndex(); ip++; break;
      case OpCode::STORE_INDEX: HandleStoreIndex(ins.arg); ip++; break;

      case OpCode::ADD:
      case OpCode::SUB:
      case OpCode::MUL:
      case OpCode::DIV:
      case OpCode::MOD:
      case OpCode::EQ:
      case OpCode::NEQ:
      case OpCode::LT:
      case OpCode::GT:
      case OpCode::LE:
      case OpCode::GE:
        HandleBinaryOp(ins.op);
        ip++;
        break;

      case OpCode::NEG:
      case OpCode::NOT:
        HandleUnaryOp(ins.op);
        ip++;
        break;

      case OpCode::JMP: {
        auto it = label_map_.find(ins.arg);
        if (it == label_map_.end()) throw std::runtime_error("unknown label for JMP: " + ins.arg);
        ip = it->second;
        break;
      }

      case OpCode::JZ: {
        Value v = Pop();
        bool cond = !v.IsZero();
        if (!cond) {
          auto it = label_map_.find(ins.arg);
          if (it == label_map_.end()) throw std::runtime_error("unknown label for JZ: " + ins.arg);
          ip = it->second;
        } else {
          ip++;
        }
        break;
      }

      case OpCode::CALL: {
        // builtins (print/input) handled directly
        if (IsBuiltin(ins.arg)) {
          CallBuiltin(ins.arg);
          ip++;
        } else {
          // call user function: push a return frame and jump to label
          Frame frame;
          frame.ret_ip = ip + 1;
          call_stack_.push_back(std::move(frame));
          auto it = label_map_.find(ins.arg);
          if (it == label_map_.end()) throw std::runtime_error("unknown function label: " + ins.arg);
          ip = it->second;
        }
        break;
      }

      case OpCode::RET: {
        if (call_stack_.empty()) return 0; // nothing to return to
        Frame fr = std::move(call_stack_.back());
        call_stack_.pop_back();
        // if we've reached bottom sentinel frame -> exit
        if (call_stack_.empty()) return 0;
        ip = fr.ret_ip;
        break;
      }

      case OpCode::LABEL:
        // no-op
        ip++;
        break;

      default:
        throw std::runtime_error("unhandled opcode");
    }
  }

  return 0;
}

// Stack helpers
void VM::Push(const Value &v) { stack_.push_back(v); }
Value VM::Pop() {
  if (stack_.empty()) throw std::runtime_error("stack underflow");
  Value v = stack_.back(); stack_.pop_back(); return v;
}
Value VM::Peek(size_t depth) const {
  if (depth >= stack_.size()) throw std::runtime_error("stack overflow peek");
  return stack_[stack_.size() - 1 - depth];
}

// simple push handlers
void VM::HandlePushInt(const std::string &arg) {
  try {
    long long v = std::stoll(arg);
    Push(Value::MakeInt(static_cast<int64_t>(v)));
  } catch (...) {
    Push(Value::MakeInt(0));
  }
}

void VM::HandlePushDouble(const std::string &arg) {
  try {
    double v = std::stod(arg);
    Push(Value::MakeDouble(v));
  } catch (...) {
    Push(Value::MakeDouble(0.0));
  }
}

void VM::HandlePushString(const std::string &arg) {
  // strip surrounding quotes if present
  std::string s = arg;
  if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
    s = s.substr(1, s.size() - 2);
  }
  Push(Value::MakeString(s));
}

void VM::HandleLoad(const std::string &name) {
  if (call_stack_.empty()) { Push(Value::MakeInt(0)); return; }
  auto &locals = call_stack_.back().locals;
  auto it = locals.find(name);
  if (it == locals.end()) {
    // not found -> push zero
    Push(Value::MakeInt(0));
    return;
  }
  Push(it->second);
}

void VM::HandleStore(const std::string &name) {
  if (call_stack_.empty()) {
    Frame f; f.ret_ip = code_.size(); call_stack_.push_back(std::move(f));
  }
  Value v = Pop();
  call_stack_.back().locals[name] = v;
}

void VM::HandleNewArray() {
  Value sizeVal = Pop();
  int64_t n = sizeVal.AsInt();
  if (n < 0) n = 0;
  Push(Value::MakeArray(static_cast<size_t>(n)));
}

void VM::HandleLoadIndex() {
  Value idx = Pop();
  Value arr = Pop();
  int64_t i = idx.AsInt();
  if (arr.type == ValueType::kArray) {
    if (i < 0 || static_cast<size_t>(i) >= arr.a.size()) Push(Value::MakeInt(0));
    else Push(arr.a[static_cast<size_t>(i)]);
  } else if (arr.type == ValueType::kString) {
    if (i < 0 || static_cast<size_t>(i) >= arr.s.size()) Push(Value::MakeString(std::string()));
    else Push(Value::MakeString(std::string(1, arr.s[static_cast<size_t>(i)])));
  } else {
    Push(Value::MakeInt(0));
  }
}

void VM::HandleStoreIndex(const std::string &varName) {
  if (!varName.empty()) {
    // new convention: stack = [..., value, index] (top is index)
    Value idx = Pop();
    Value val = Pop();
    int64_t i = idx.AsInt();

    if (call_stack_.empty()) throw std::runtime_error("store_index with no frame");
    auto &locals = call_stack_.back().locals;
    auto it = locals.find(varName);
    if (it == locals.end()) {
      // if variable not present, create array large enough
      size_t needed = static_cast<size_t>(std::max<int64_t>(0, i + 1));
      locals[varName] = Value::MakeArray(needed);
      it = locals.find(varName);
    }

    if (it->second.type != ValueType::kArray) {
      // convert/replace with array
      it->second = Value::MakeArray(static_cast<size_t>(std::max<int64_t>(0, i + 1)));
    }

    if (i >= 0) {
      if (static_cast<size_t>(i) >= it->second.a.size())
        it->second.a.resize(static_cast<size_t>(i) + 1, Value::MakeInt(0));
      it->second.a[static_cast<size_t>(i)] = val;
    }

    // nothing pushed
  } else {
    // fallback for older convention: stack = [..., value, array, index]
    // previous behaviour popped index, array, value and pushed modified array back
    Value idx = Pop();
    Value arr = Pop();
    Value val = Pop();
    int64_t i = idx.AsInt();
    if (arr.type == ValueType::kArray) {
      if (i >= 0 && static_cast<size_t>(i) < arr.a.size()) {
        arr.a[static_cast<size_t>(i)] = val;
        Push(arr);
      } else {
        Push(arr);
      }
    } else {
      Push(val);
    }
  }
}

void VM::HandleBinaryOp(OpCode op) {
  Value b = Pop();
  Value a = Pop();

  // Determine numeric vs string
  bool a_is_double = (a.type == ValueType::kDouble);
  bool b_is_double = (b.type == ValueType::kDouble);
  bool either_double = a_is_double || b_is_double;

  switch (op) {
    case OpCode::ADD: {
      if (a.type == ValueType::kString || b.type == ValueType::kString) {
        // string concatenation
        std::string s = a.AsString() + b.AsString();
        Push(Value::MakeString(s));
      } else if (either_double) {
        Push(Value::MakeDouble(a.AsDouble() + b.AsDouble()));
      } else {
        Push(Value::MakeInt(a.AsInt() + b.AsInt()));
      }
      break;
    }

    case OpCode::SUB:
      if (either_double) Push(Value::MakeDouble(a.AsDouble() - b.AsDouble()));
      else Push(Value::MakeInt(a.AsInt() - b.AsInt()));
      break;
    case OpCode::MUL:
      if (either_double) Push(Value::MakeDouble(a.AsDouble() * b.AsDouble()));
      else Push(Value::MakeInt(a.AsInt() * b.AsInt()));
      break;
    case OpCode::DIV:
      if (either_double) Push(Value::MakeDouble(a.AsDouble() / b.AsDouble()));
      else {
        int64_t ib = b.AsInt();
        if (ib == 0) Push(Value::MakeInt(0));
        else Push(Value::MakeInt(a.AsInt() / ib));
      }
      break;

    case OpCode::MOD: {
      int64_t ib = b.AsInt();
      if (ib == 0) Push(Value::MakeInt(0)); // Avoid division by zero
      else Push(Value::MakeInt(a.AsInt() % ib));
    }
      break;

    case OpCode::EQ: Push(Value::MakeInt(a.AsString() == b.AsString() ? 1 : 0)); break;
    case OpCode::NEQ: Push(Value::MakeInt(a.AsString() != b.AsString() ? 1 : 0)); break;

    case OpCode::LT:
      if (either_double) Push(Value::MakeInt(a.AsDouble() < b.AsDouble() ? 1 : 0));
      else Push(Value::MakeInt(a.AsInt() < b.AsInt() ? 1 : 0));
      break;
    case OpCode::GT:
      if (either_double) Push(Value::MakeInt(a.AsDouble() > b.AsDouble() ? 1 : 0));
      else Push(Value::MakeInt(a.AsInt() > b.AsInt() ? 1 : 0));
      break;
    case OpCode::LE:
      if (either_double) Push(Value::MakeInt(a.AsDouble() <= b.AsDouble() ? 1 : 0));
      else Push(Value::MakeInt(a.AsInt() <= b.AsInt() ? 1 : 0));
      break;
    case OpCode::GE:
      if (either_double) Push(Value::MakeInt(a.AsDouble() >= b.AsDouble() ? 1 : 0));
      else Push(Value::MakeInt(a.AsInt() >= b.AsInt() ? 1 : 0));
      break;

    default:
      throw std::runtime_error("invalid binary op");
  }
}

void VM::HandleUnaryOp(OpCode op) {
  Value v = Pop();
  switch (op) {
    case OpCode::NEG:
      if (v.type == ValueType::kDouble) Push(Value::MakeDouble(-v.d));
      else Push(Value::MakeInt(-v.AsInt()));
      break;
    case OpCode::NOT:
      Push(Value::MakeInt(v.IsZero() ? 1 : 0));
      break;
    default:
      throw std::runtime_error("invalid unary op");
  }
}

bool VM::IsBuiltin(const std::string &name) const {
  return name == "printInt" || name == "printStr" || name == "printDouble" ||
         name == "inputInt" || name == "inputStr" || name == "inputDouble" || name == "vsuprun";
}

void VM::CallBuiltin(const std::string &name) {
  if (name == "printInt") {
    Value v = Pop();
    std::cout << v.AsInt();
  } else if (name == "printDouble") {
    Value v = Pop();
    std::cout << v.AsDouble();
  } else if (name == "printStr") {
    Value v = Pop();
    std::cout << v.AsString();
  } else if (name == "inputInt") {
    long long x = 0; if (!(std::cin >> x)) x = 0; Push(Value::MakeInt(static_cast<int64_t>(x)));
  } else if (name == "inputDouble") {
    double d = 0.0; if (!(std::cin >> d)) d = 0.0; Push(Value::MakeDouble(d));
  } else if (name == "inputStr") {
    std::string s; std::getline(std::cin, s);
    // if previous extraction left a newline, try again
    if (s.empty() && std::cin.good()) std::getline(std::cin, s);
    Push(Value::MakeString(s));
  } else if (name == "vsuprun") {
    Push(Value::MakeInt(1.0 * clock() / CLOCKS_PER_SEC >= 1.95));
  }
}
