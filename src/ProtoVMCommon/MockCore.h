#ifndef MOCK_CORE_H
#define MOCK_CORE_H

// Mock U++ Core library types to allow compilation without U++
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <iostream>
#include <sstream>
#include <algorithm>

// Basic U++ types
using String = std::string;

template<typename T>
using Vector = std::vector<T>;

template<typename K, typename V>
using Map = std::map<K, V>;

using byte = unsigned char;
using dword = unsigned int;

// Mock U++ Value type
struct Value {
    enum Type { NONE, INT, DOUBLE, STRING, BOOL };
    
    Type type = NONE;
    union {
        int int_val;
        double double_val;
        bool bool_val;
    };
    std::string string_val;

    Value() : type(NONE) {}
    Value(int v) : type(INT), int_val(v) {}
    Value(double v) : type(DOUBLE), double_val(v) {}
    Value(const std::string& v) : type(STRING), string_val(v) {}
    Value(const char* v) : type(STRING), string_val(v) {}
    Value(bool v) : type(BOOL), bool_val(v) {}

    int ToInt() const { return type == INT ? int_val : 0; }
    double ToDouble() const { return type == DOUBLE ? double_val : 0.0; }
    std::string ToString() const { return type == STRING ? string_val : std::string(""); }
    bool ToBool() const { return type == BOOL ? bool_val : false; }
    bool IsNull() const { return type == NONE; }
    
    std::string ToStd() const { return ToString(); }
};

// Mock U++ ValueMap (similar to std::map<String, Value>)
struct ValueMap {
    std::map<std::string, Value> data;
    
    Value& Add(const std::string& key, const Value& value) {
        data[key] = value;
        return data[key];
    }
    
    const Value& Get(const std::string& key, const Value& default_val = Value{}) const {
        auto it = data.find(key);
        if (it != data.end()) {
            return it->second;
        }
        return default_val;
    }
    
    Value& Get(const std::string& key, const Value& default_val = Value{}) {
        auto it = data.find(key);
        if (it != data.end()) {
            return it->second;
        }
        return const_cast<Value&>(default_val);
    }
};

// Mock LOG macro
#define LOG(x) std::cout << x << std::endl;

// Mock ASSERT
#define ASSERT(condition) do { if (!(condition)) { std::cout << "ASSERTION FAILED: " << #condition << std::endl; } } while(0)

// Common U++ functions
inline int StrInt(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (...) {
        return 0;
    }
}

inline std::string AsString(int n) {
    return std::to_string(n);
}

inline std::string ToLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

inline Vector<String> Split(const String& str, char delimiter, bool skip_empty) {
    Vector<String> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        if (!skip_empty || !item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

inline String HexStr(int value) {
    std::stringstream ss;
    ss << std::hex << value;
    return ss.str();
}

#endif // MOCK_CORE_H