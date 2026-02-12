#pragma once
#include <string>

enum class OpCode {
  PUSH_INT,
  PUSH_STRING,

  LOAD,
  STORE,

  LOAD_INDEX,
  STORE_INDEX,

  ADD,
  SUB,
  MUL,
  DIV,
  NEG,

  EQ,
  NEQ,
  LT,
  GT,
  LE,
  GE,

  NOT,

  JMP,
  JZ,

  CALL,
  RET,

  LABEL
};

struct Instruction {
  OpCode op;
  std::string arg;

  Instruction(OpCode o, const std::string& a = "")
      : op(o), arg(a) {}
};


inline std::string OpCodeToString(OpCode op) {
  switch (op) {
    case OpCode::PUSH_INT: return "PUSH_INT";
    case OpCode::PUSH_STRING: return "PUSH_STRING";

    case OpCode::LOAD: return "LOAD";
    case OpCode::STORE: return "STORE";

    case OpCode::LOAD_INDEX: return "LOAD_INDEX";
    case OpCode::STORE_INDEX: return "STORE_INDEX";

    case OpCode::ADD: return "ADD";
    case OpCode::SUB: return "SUB";
    case OpCode::MUL: return "MUL";
    case OpCode::DIV: return "DIV";
    case OpCode::NEG: return "NEG";

    case OpCode::EQ: return "EQ";
    case OpCode::NEQ: return "NEQ";
    case OpCode::LT: return "LT";
    case OpCode::GT: return "GT";
    case OpCode::LE: return "LE";
    case OpCode::GE: return "GE";

    case OpCode::NOT: return "NOT";

    case OpCode::JMP: return "JMP";
    case OpCode::JZ: return "JZ";

    case OpCode::CALL: return "CALL";
    case OpCode::RET: return "RET";

    case OpCode::LABEL: return "LABEL";
  }
  return "UNKNOWN";
}
