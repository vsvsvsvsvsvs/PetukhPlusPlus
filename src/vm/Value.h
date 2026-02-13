#ifndef PETUKH_VALUE_H
#define PETUKH_VALUE_H

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

enum class ValueType { kNone, kInt, kDouble, kString, kArray };

struct Value {
  ValueType type = ValueType::kNone;
  int64_t i = 0;
  double d = 0.0;
  std::string s;
  std::vector<Value> a;  // used when type == kArray

  Value() : type(ValueType::kNone), i(0), d(0.0) {}
  static Value MakeInt(int64_t v) { Value x; x.type = ValueType::kInt; x.i = v; return x; }
  static Value MakeDouble(double v) { Value x; x.type = ValueType::kDouble; x.d = v; return x; }
  static Value MakeString(const std::string &v) { Value x; x.type = ValueType::kString; x.s = v; return x; }
  static Value MakeArray(size_t n) { Value x; x.type = ValueType::kArray; x.a.assign(n, Value::MakeInt(0)); return x; }

  bool IsZero() const {
    switch (type) {
      case ValueType::kInt: return i == 0;
      case ValueType::kDouble: return d == 0.0;
      case ValueType::kString: return s.empty();
      case ValueType::kArray: return a.empty();
      default: return true;
    }
  }

  double AsDouble() const {
    switch (type) {
      case ValueType::kInt: return static_cast<double>(i);
      case ValueType::kDouble: return d;
      case ValueType::kString: try { return std::stod(s); } catch(...) { return 0.0; }
      default: return 0.0;
    }
  }

  int64_t AsInt() const {
    switch (type) {
      case ValueType::kInt: return i;
      case ValueType::kDouble: return static_cast<int64_t>(d);
      case ValueType::kString: try { return std::stoll(s); } catch(...) { return 0; }
      default: return 0;
    }
  }

  std::string AsString() const {
    switch (type) {
      case ValueType::kInt: return std::to_string(i);
      case ValueType::kDouble: {
        std::ostringstream oss; oss << d; return oss.str();
      }
      case ValueType::kString: return s;
      default: return std::string();
    }
  }
};

#endif  // PETUKH_VALUE_H
