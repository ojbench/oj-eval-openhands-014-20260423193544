#include "Evalvisitor.h"
#include "Python3Lexer.h"
#include "Python3Parser.h"
#include <algorithm>
#include <stdexcept>

using namespace antlr4;

// BigInt implementation
BigInt::BigInt() : negative(false) {
    digits.push_back(0);
}

BigInt::BigInt(long long n) : negative(n < 0) {
    if (n == 0) {
        digits.push_back(0);
        return;
    }
    n = std::abs(n);
    while (n > 0) {
        digits.push_back(n % 10);
        n /= 10;
    }
}

BigInt::BigInt(const std::string& s) {
    if (s.empty() || s == "-") {
        digits.push_back(0);
        negative = false;
        return;
    }
    
    size_t start = 0;
    negative = (s[0] == '-');
    if (negative || s[0] == '+') start = 1;
    
    for (int i = s.length() - 1; i >= (int)start; i--) {
        if (s[i] >= '0' && s[i] <= '9') {
            digits.push_back(s[i] - '0');
        }
    }
    
    if (digits.empty()) digits.push_back(0);
    removeLeadingZeros();
}

void BigInt::removeLeadingZeros() {
    while (digits.size() > 1 && digits.back() == 0) {
        digits.pop_back();
    }
    if (digits.size() == 1 && digits[0] == 0) {
        negative = false;
    }
}

BigInt BigInt::operator+(const BigInt& other) const {
    if (negative != other.negative) {
        if (negative) {
            return other - (-(*this));
        } else {
            return *this - (-other);
        }
    }
    
    BigInt result;
    result.negative = negative;
    result.digits.clear();
    
    int carry = 0;
    size_t maxLen = std::max(digits.size(), other.digits.size());
    
    for (size_t i = 0; i < maxLen || carry; i++) {
        int sum = carry;
        if (i < digits.size()) sum += digits[i];
        if (i < other.digits.size()) sum += other.digits[i];
        
        result.digits.push_back(sum % 10);
        carry = sum / 10;
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInt BigInt::operator-(const BigInt& other) const {
    if (negative != other.negative) {
        if (negative) {
            return -((-(*this)) + other);
        } else {
            return *this + (-other);
        }
    }
    
    if (negative) {
        return (-other) - (-(*this));
    }
    
    if (*this < other) {
        return -(other - *this);
    }
    
    BigInt result;
    result.negative = false;
    result.digits.clear();
    
    int borrow = 0;
    for (size_t i = 0; i < digits.size(); i++) {
        int diff = digits[i] - borrow;
        if (i < other.digits.size()) {
            diff -= other.digits[i];
        }
        
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        
        result.digits.push_back(diff);
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInt BigInt::operator*(const BigInt& other) const {
    BigInt result;
    result.digits.assign(digits.size() + other.digits.size(), 0);
    result.negative = (negative != other.negative);
    
    for (size_t i = 0; i < digits.size(); i++) {
        int carry = 0;
        for (size_t j = 0; j < other.digits.size() || carry; j++) {
            long long cur = result.digits[i + j] + 
                           carry + 
                           (j < other.digits.size() ? (long long)digits[i] * other.digits[j] : 0);
            result.digits[i + j] = cur % 10;
            carry = cur / 10;
        }
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInt BigInt::operator/(const BigInt& other) const {
    if (other.isZero()) {
        throw std::runtime_error("Division by zero");
    }
    
    BigInt dividend = this->abs();
    BigInt divisor = other.abs();
    
    if (dividend < divisor) {
        if (negative != other.negative && !dividend.isZero()) {
            return BigInt(-1);
        }
        return BigInt(0);
    }
    
    std::string result_str = "";
    BigInt current(0);
    
    for (int i = dividend.digits.size() - 1; i >= 0; i--) {
        current.digits.insert(current.digits.begin(), dividend.digits[i]);
        current.removeLeadingZeros();
        
        int count = 0;
        while (!(current < divisor)) {
            current = current - divisor;
            count++;
        }
        result_str += (char)('0' + count);
    }
    
    BigInt result(result_str);
    result.negative = (negative != other.negative) && !result.isZero();
    
    if (negative != other.negative && !(dividend % divisor).isZero()) {
        result = result - BigInt(1);
    }
    
    return result;
}

BigInt BigInt::operator%(const BigInt& other) const {
    if (other.isZero()) {
        throw std::runtime_error("Modulo by zero");
    }
    
    BigInt quotient = *this / other;
    BigInt result = *this - (quotient * other);
    
    return result;
}

BigInt BigInt::operator-() const {
    BigInt result = *this;
    if (!isZero()) {
        result.negative = !negative;
    }
    return result;
}

bool BigInt::operator<(const BigInt& other) const {
    if (negative != other.negative) {
        return negative;
    }
    
    if (digits.size() != other.digits.size()) {
        return negative ? (digits.size() > other.digits.size()) : (digits.size() < other.digits.size());
    }
    
    for (int i = digits.size() - 1; i >= 0; i--) {
        if (digits[i] != other.digits[i]) {
            return negative ? (digits[i] > other.digits[i]) : (digits[i] < other.digits[i]);
        }
    }
    
    return false;
}

bool BigInt::operator>(const BigInt& other) const {
    return other < *this;
}

bool BigInt::operator<=(const BigInt& other) const {
    return !(other < *this);
}

bool BigInt::operator>=(const BigInt& other) const {
    return !(*this < other);
}

bool BigInt::operator==(const BigInt& other) const {
    return negative == other.negative && digits == other.digits;
}

bool BigInt::operator!=(const BigInt& other) const {
    return !(*this == other);
}

std::string BigInt::toString() const {
    std::string result = "";
    if (negative && !isZero()) result += "-";
    for (int i = digits.size() - 1; i >= 0; i--) {
        result += (char)('0' + digits[i]);
    }
    return result;
}

double BigInt::toDouble() const {
    double result = 0;
    double multiplier = 1;
    for (size_t i = 0; i < digits.size(); i++) {
        result += digits[i] * multiplier;
        multiplier *= 10;
    }
    return negative ? -result : result;
}

bool BigInt::isZero() const {
    return digits.size() == 1 && digits[0] == 0;
}

bool BigInt::isNegative() const {
    return negative && !isZero();
}

BigInt BigInt::abs() const {
    BigInt result = *this;
    result.negative = false;
    return result;
}

// Value implementation
Value::Value() : type(ValueType::NONE) {}

Value::Value(bool b) : type(ValueType::BOOL), boolVal(b) {}

Value::Value(const BigInt& i) : type(ValueType::INT), intVal(i) {}

Value::Value(double f) : type(ValueType::FLOAT), floatVal(f) {}

Value::Value(const std::string& s) : type(ValueType::STRING), strVal(s) {}

Value::Value(const std::vector<Value>& t) : type(ValueType::TUPLE), tupleVal(t) {}

std::string Value::toString() const {
    std::ostringstream oss;
    switch (type) {
        case ValueType::NONE:
            return "None";
        case ValueType::BOOL:
            return boolVal ? "True" : "False";
        case ValueType::INT:
            return intVal.toString();
        case ValueType::FLOAT:
            oss << std::fixed << std::setprecision(6) << floatVal;
            return oss.str();
        case ValueType::STRING:
            return strVal;
        case ValueType::TUPLE: {
            std::string result = "(";
            for (size_t i = 0; i < tupleVal.size(); i++) {
                if (i > 0) result += ", ";
                result += tupleVal[i].toString();
            }
            if (tupleVal.size() == 1) result += ",";
            result += ")";
            return result;
        }
        default:
            return "";
    }
}

bool Value::toBool() const {
    switch (type) {
        case ValueType::NONE:
            return false;
        case ValueType::BOOL:
            return boolVal;
        case ValueType::INT:
            return !intVal.isZero();
        case ValueType::FLOAT:
            return floatVal != 0.0;
        case ValueType::STRING:
            return !strVal.empty();
        case ValueType::TUPLE:
            return !tupleVal.empty();
        default:
            return false;
    }
}

BigInt Value::toInt() const {
    switch (type) {
        case ValueType::BOOL:
            return BigInt(boolVal ? 1 : 0);
        case ValueType::INT:
            return intVal;
        case ValueType::FLOAT:
            return BigInt((long long)floatVal);
        case ValueType::STRING:
            return BigInt(strVal);
        default:
            return BigInt(0);
    }
}

double Value::toFloat() const {
    switch (type) {
        case ValueType::BOOL:
            return boolVal ? 1.0 : 0.0;
        case ValueType::INT:
            return intVal.toDouble();
        case ValueType::FLOAT:
            return floatVal;
        case ValueType::STRING:
            return std::stod(strVal);
        default:
            return 0.0;
    }
}

Value Value::operator+(const Value& other) const {
    if (type == ValueType::STRING || other.type == ValueType::STRING) {
        if (type == ValueType::STRING && other.type == ValueType::STRING) {
            return Value(strVal + other.strVal);
        }
    }
    
    if (type == ValueType::STRING && other.type == ValueType::INT) {
        std::string result = "";
        BigInt count = other.intVal;
        BigInt zero(0);
        while (count > zero) {
            result += strVal;
            count = count - BigInt(1);
        }
        return Value(result);
    }
    
    if (type == ValueType::INT && other.type == ValueType::STRING) {
        return other + *this;
    }
    
    if (type == ValueType::FLOAT || other.type == ValueType::FLOAT) {
        return Value(toFloat() + other.toFloat());
    }
    
    if (type == ValueType::INT && other.type == ValueType::INT) {
        return Value(intVal + other.intVal);
    }
    
    return Value();
}

Value Value::operator-(const Value& other) const {
    if (type == ValueType::FLOAT || other.type == ValueType::FLOAT) {
        return Value(toFloat() - other.toFloat());
    }
    
    if (type == ValueType::INT && other.type == ValueType::INT) {
        return Value(intVal - other.intVal);
    }
    
    return Value();
}

Value Value::operator*(const Value& other) const {
    if (type == ValueType::STRING && other.type == ValueType::INT) {
        std::string result = "";
        BigInt count = other.intVal;
        BigInt zero(0);
        while (count > zero) {
            result += strVal;
            count = count - BigInt(1);
        }
        return Value(result);
    }
    
    if (type == ValueType::INT && other.type == ValueType::STRING) {
        return other * *this;
    }
    
    if (type == ValueType::FLOAT || other.type == ValueType::FLOAT) {
        return Value(toFloat() * other.toFloat());
    }
    
    if (type == ValueType::INT && other.type == ValueType::INT) {
        return Value(intVal * other.intVal);
    }
    
    return Value();
}

Value Value::operator/(const Value& other) const {
    return Value(toFloat() / other.toFloat());
}

Value Value::operator%(const Value& other) const {
    if (type == ValueType::INT && other.type == ValueType::INT) {
        return Value(intVal % other.intVal);
    }
    return Value();
}

Value Value::operator-() const {
    if (type == ValueType::INT) {
        return Value(-intVal);
    } else if (type == ValueType::FLOAT) {
        return Value(-floatVal);
    } else if (type == ValueType::BOOL) {
        return Value(-(BigInt(boolVal ? 1 : 0)));
    }
    return Value();
}

bool Value::operator<(const Value& other) const {
    if (type == ValueType::STRING && other.type == ValueType::STRING) {
        return strVal < other.strVal;
    }
    
    if (type == ValueType::FLOAT || other.type == ValueType::FLOAT) {
        return toFloat() < other.toFloat();
    }
    
    if (type == ValueType::INT && other.type == ValueType::INT) {
        return intVal < other.intVal;
    }
    
    return false;
}

bool Value::operator>(const Value& other) const {
    return other < *this;
}

bool Value::operator<=(const Value& other) const {
    return !(other < *this);
}

bool Value::operator>=(const Value& other) const {
    return !(*this < other);
}

bool Value::operator==(const Value& other) const {
    if (type == ValueType::STRING && other.type == ValueType::STRING) {
        return strVal == other.strVal;
    }
    
    if (type == ValueType::NONE || other.type == ValueType::NONE) {
        return type == other.type;
    }
    
    if (type == ValueType::STRING || other.type == ValueType::STRING) {
        return false;
    }
    
    if (type == ValueType::FLOAT || other.type == ValueType::FLOAT) {
        return toFloat() == other.toFloat();
    }
    
    if (type == ValueType::INT && other.type == ValueType::INT) {
        return intVal == other.intVal;
    }
    
    if (type == ValueType::BOOL && other.type == ValueType::BOOL) {
        return boolVal == other.boolVal;
    }
    
    if ((type == ValueType::BOOL || type == ValueType::INT) && 
        (other.type == ValueType::BOOL || other.type == ValueType::INT)) {
        return toInt() == other.toInt();
    }
    
    return false;
}

bool Value::operator!=(const Value& other) const {
    return !(*this == other);
}

// EvalVisitor implementation
EvalVisitor::EvalVisitor() : flowControl(FlowControl::NONE) {}

void EvalVisitor::enterScope() {
    scopes.push_back(std::map<std::string, Value>());
}

void EvalVisitor::exitScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

Value EvalVisitor::getVariable(const std::string& name) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
        if (scopes[i].find(name) != scopes[i].end()) {
            return scopes[i][name];
        }
    }
    
    if (globalScope.find(name) != globalScope.end()) {
        return globalScope[name];
    }
    
    return Value();
}

void EvalVisitor::setVariable(const std::string& name, const Value& value) {
    if (!scopes.empty()) {
        scopes.back()[name] = value;
    } else {
        globalScope[name] = value;
    }
}

bool EvalVisitor::hasVariable(const std::string& name) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
        if (scopes[i].find(name) != scopes[i].end()) {
            return true;
        }
    }
    return globalScope.find(name) != globalScope.end();
}

Value EvalVisitor::builtinPrint(const std::vector<Value>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) std::cout << " ";
        std::cout << args[i].toString();
    }
    std::cout << std::endl;
    return Value();
}

Value EvalVisitor::builtinInt(const Value& arg) {
    return Value(arg.toInt());
}

Value EvalVisitor::builtinFloat(const Value& arg) {
    return Value(arg.toFloat());
}

Value EvalVisitor::builtinStr(const Value& arg) {
    return Value(arg.toString());
}

Value EvalVisitor::builtinBool(const Value& arg) {
    return Value(arg.toBool());
}

std::string EvalVisitor::processFormatString(const std::string& fstr) {
    std::string result = "";
    size_t i = 0;
    
    while (i < fstr.length()) {
        if (fstr[i] == '{') {
            if (i + 1 < fstr.length() && fstr[i + 1] == '{') {
                result += '{';
                i += 2;
                continue;
            }
            
            size_t end = i + 1;
            int depth = 1;
            while (end < fstr.length() && depth > 0) {
                if (fstr[end] == '{') depth++;
                else if (fstr[end] == '}') depth--;
                end++;
            }
            
            if (depth == 0) {
                std::string expr = fstr.substr(i + 1, end - i - 2);
                
                antlr4::ANTLRInputStream input(expr);
                Python3Lexer lexer(&input);
                antlr4::CommonTokenStream tokens(&lexer);
                tokens.fill();
                Python3Parser parser(&tokens);
                
                auto tree = parser.test();
                Value val = std::any_cast<Value>(visit(tree));
                result += val.toString();
                
                i = end;
                continue;
            }
        } else if (fstr[i] == '}') {
            if (i + 1 < fstr.length() && fstr[i + 1] == '}') {
                result += '}';
                i += 2;
                continue;
            }
        }
        
        result += fstr[i];
        i++;
    }
    
    return result;
}

Value EvalVisitor::callFunction(const std::string& name, const std::vector<Value>& args, 
                                const std::map<std::string, Value>& kwargs) {
    if (name == "print") {
        return builtinPrint(args);
    } else if (name == "int") {
        if (!args.empty()) return builtinInt(args[0]);
        return Value(BigInt(0));
    } else if (name == "float") {
        if (!args.empty()) return builtinFloat(args[0]);
        return Value(0.0);
    } else if (name == "str") {
        if (!args.empty()) return builtinStr(args[0]);
        return Value("");
    } else if (name == "bool") {
        if (!args.empty()) return builtinBool(args[0]);
        return Value(false);
    }
    
    Value funcVal = getVariable(name);
    if (funcVal.type != ValueType::FUNCTION || !funcVal.funcVal) {
        return Value();
    }
    
    auto func = funcVal.funcVal;
    
    enterScope();
    
    // Don't use captured scope, just use current global scope
    // This allows access to all global variables defined before or after function definition
    
    size_t argIdx = 0;
    for (size_t i = 0; i < func->params.size(); i++) {
        const std::string& param = func->params[i];
        
        if (kwargs.find(param) != kwargs.end()) {
            setVariable(param, kwargs.at(param));
        } else if (argIdx < args.size()) {
            setVariable(param, args[argIdx++]);
        } else {
            size_t defaultIdx = i - (func->params.size() - func->defaults.size());
            if (defaultIdx < func->defaults.size()) {
                setVariable(param, func->defaults[defaultIdx]);
            }
        }
    }
    
    visit(func->body);
    
    Value result = returnValue;
    returnValue = Value();
    flowControl = FlowControl::NONE;
    
    exitScope();
    
    return result;
}

std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
        if (flowControl != FlowControl::NONE) break;
    }
    return Value();
}

std::any EvalVisitor::visitFuncdef(Python3Parser::FuncdefContext *ctx) {
    std::string name = ctx->NAME()->getText();
    
    auto func = std::make_shared<Function>();
    func->name = name;
    func->body = ctx->suite();
    
    if (ctx->parameters() && ctx->parameters()->typedargslist()) {
        auto argsCtx = ctx->parameters()->typedargslist();
        
        for (auto tfpdef : argsCtx->tfpdef()) {
            func->params.push_back(tfpdef->NAME()->getText());
        }
        
        for (auto test : argsCtx->test()) {
            Value defVal = std::any_cast<Value>(visit(test));
            func->defaults.push_back(defVal);
        }
    }
    
    Value funcValue;
    funcValue.type = ValueType::FUNCTION;
    funcValue.funcVal = func;
    
    setVariable(name, funcValue);
    
    return Value();
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    if (ctx->simple_stmt()) {
        return visit(ctx->simple_stmt());
    } else if (ctx->compound_stmt()) {
        return visit(ctx->compound_stmt());
    }
    return Value();
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    if (ctx->small_stmt()) {
        visit(ctx->small_stmt());
    }
    return Value();
}

std::any EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) {
    if (ctx->expr_stmt()) {
        return visit(ctx->expr_stmt());
    } else if (ctx->flow_stmt()) {
        return visit(ctx->flow_stmt());
    }
    return Value();
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    auto testlists = ctx->testlist();
    
    if (testlists.size() == 1) {
        return visit(testlists[0]);
    }
    
    if (ctx->augassign()) {
        auto leftTestlist = testlists[0];
        auto leftTests = leftTestlist->test();
        if (!leftTests.empty()) {
            std::string varName = leftTests[0]->getText();
            Value currentVal = getVariable(varName);
            Value rightVal = std::any_cast<Value>(visit(testlists[1]));
            
            std::string op = ctx->augassign()->getText();
            Value result;
            
            if (op == "+=") result = currentVal + rightVal;
            else if (op == "-=") result = currentVal - rightVal;
            else if (op == "*=") result = currentVal * rightVal;
            else if (op == "/=") result = currentVal / rightVal;
            else if (op == "//=") {
                if (currentVal.type == ValueType::INT && rightVal.type == ValueType::INT) {
                    result = Value(currentVal.intVal / rightVal.intVal);
                } else {
                    result = Value(std::floor(currentVal.toFloat() / rightVal.toFloat()));
                }
            } else if (op == "%=") result = currentVal % rightVal;
            
            setVariable(varName, result);
            return result;
        }
    }
    
    Value rightVal = std::any_cast<Value>(visit(testlists.back()));
    
    for (int i = testlists.size() - 2; i >= 0; i--) {
        auto leftTests = testlists[i]->test();
        
        if (leftTests.size() > 1) {
            std::vector<Value> values;
            
            if (rightVal.type == ValueType::TUPLE) {
                values = rightVal.tupleVal;
            } else {
                values.push_back(rightVal);
            }
            
            for (size_t j = 0; j < leftTests.size() && j < values.size(); j++) {
                std::string varName = leftTests[j]->getText();
                setVariable(varName, values[j]);
            }
        } else if (!leftTests.empty()) {
            std::string varName = leftTests[0]->getText();
            setVariable(varName, rightVal);
        }
    }
    
    return rightVal;
}

std::any EvalVisitor::visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) {
    if (ctx->break_stmt()) {
        return visit(ctx->break_stmt());
    } else if (ctx->continue_stmt()) {
        return visit(ctx->continue_stmt());
    } else if (ctx->return_stmt()) {
        return visit(ctx->return_stmt());
    }
    return Value();
}

std::any EvalVisitor::visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) {
    flowControl = FlowControl::BREAK;
    return Value();
}

std::any EvalVisitor::visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) {
    flowControl = FlowControl::CONTINUE;
    return Value();
}

std::any EvalVisitor::visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) {
    flowControl = FlowControl::RETURN;
    
    if (ctx->testlist()) {
        auto tests = ctx->testlist()->test();
        if (tests.size() == 1) {
            returnValue = std::any_cast<Value>(visit(tests[0]));
        } else {
            std::vector<Value> values;
            for (auto test : tests) {
                values.push_back(std::any_cast<Value>(visit(test)));
            }
            returnValue = Value(values);
        }
    } else {
        returnValue = Value();
    }
    
    return returnValue;
}

std::any EvalVisitor::visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) {
    if (ctx->if_stmt()) {
        return visit(ctx->if_stmt());
    } else if (ctx->while_stmt()) {
        return visit(ctx->while_stmt());
    } else if (ctx->funcdef()) {
        return visit(ctx->funcdef());
    }
    return Value();
}

std::any EvalVisitor::visitIf_stmt(Python3Parser::If_stmtContext *ctx) {
    auto tests = ctx->test();
    auto suites = ctx->suite();
    
    for (size_t i = 0; i < tests.size(); i++) {
        Value condition = std::any_cast<Value>(visit(tests[i]));
        if (condition.toBool()) {
            visit(suites[i]);
            return Value();
        }
    }
    
    if (suites.size() > tests.size()) {
        visit(suites.back());
    }
    
    return Value();
}

std::any EvalVisitor::visitWhile_stmt(Python3Parser::While_stmtContext *ctx) {
    while (true) {
        Value condition = std::any_cast<Value>(visit(ctx->test()));
        if (!condition.toBool()) break;
        
        visit(ctx->suite());
        
        if (flowControl == FlowControl::BREAK) {
            flowControl = FlowControl::NONE;
            break;
        } else if (flowControl == FlowControl::CONTINUE) {
            flowControl = FlowControl::NONE;
            continue;
        } else if (flowControl == FlowControl::RETURN) {
            break;
        }
    }
    
    return Value();
}

std::any EvalVisitor::visitSuite(Python3Parser::SuiteContext *ctx) {
    if (ctx->simple_stmt()) {
        return visit(ctx->simple_stmt());
    }
    
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
        if (flowControl != FlowControl::NONE) break;
    }
    
    return Value();
}

std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) {
    return visit(ctx->or_test());
}

std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) {
    auto andTests = ctx->and_test();
    
    Value result = std::any_cast<Value>(visit(andTests[0]));
    
    for (size_t i = 1; i < andTests.size(); i++) {
        if (result.toBool()) {
            return result;
        }
        result = std::any_cast<Value>(visit(andTests[i]));
    }
    
    return result;
}

std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) {
    auto notTests = ctx->not_test();
    
    Value result = std::any_cast<Value>(visit(notTests[0]));
    
    for (size_t i = 1; i < notTests.size(); i++) {
        if (!result.toBool()) {
            return result;
        }
        result = std::any_cast<Value>(visit(notTests[i]));
    }
    
    return result;
}

std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) {
    if (ctx->NOT()) {
        Value val = std::any_cast<Value>(visit(ctx->not_test()));
        return Value(!val.toBool());
    }
    return visit(ctx->comparison());
}

std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) {
    auto arithExprs = ctx->arith_expr();
    
    if (arithExprs.size() == 1) {
        return visit(arithExprs[0]);
    }
    
    std::vector<Value> values;
    for (auto expr : arithExprs) {
        values.push_back(std::any_cast<Value>(visit(expr)));
    }
    
    auto compOps = ctx->comp_op();
    
    for (size_t i = 0; i < compOps.size(); i++) {
        std::string op = compOps[i]->getText();
        bool result;
        
        if (op == "<") result = values[i] < values[i + 1];
        else if (op == ">") result = values[i] > values[i + 1];
        else if (op == "<=") result = values[i] <= values[i + 1];
        else if (op == ">=") result = values[i] >= values[i + 1];
        else if (op == "==") result = values[i] == values[i + 1];
        else if (op == "!=") result = values[i] != values[i + 1];
        else result = false;
        
        if (!result) {
            return Value(false);
        }
    }
    
    return Value(true);
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    auto terms = ctx->term();
    
    Value result = std::any_cast<Value>(visit(terms[0]));
    
    auto ops = ctx->addorsub_op();
    for (size_t i = 0; i < ops.size(); i++) {
        Value right = std::any_cast<Value>(visit(terms[i + 1]));
        std::string op = ops[i]->getText();
        
        if (op == "+") {
            result = result + right;
        } else if (op == "-") {
            result = result - right;
        }
    }
    
    return result;
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    auto factors = ctx->factor();
    
    Value result = std::any_cast<Value>(visit(factors[0]));
    
    auto ops = ctx->muldivmod_op();
    for (size_t i = 0; i < ops.size(); i++) {
        Value right = std::any_cast<Value>(visit(factors[i + 1]));
        std::string op = ops[i]->getText();
        
        if (op == "*") {
            result = result * right;
        } else if (op == "/") {
            result = result / right;
        } else if (op == "//") {
            if (result.type == ValueType::INT && right.type == ValueType::INT) {
                result = Value(result.intVal / right.intVal);
            } else {
                result = Value(std::floor(result.toFloat() / right.toFloat()));
            }
        } else if (op == "%") {
            result = result % right;
        }
    }
    
    return result;
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    if (ctx->ADD()) {
        return visit(ctx->factor());
    } else if (ctx->MINUS()) {
        Value val = std::any_cast<Value>(visit(ctx->factor()));
        return -val;
    }
    return visit(ctx->atom_expr());
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    Value result = std::any_cast<Value>(visit(ctx->atom()));
    
    auto trailer = ctx->trailer();
    if (!trailer) {
        return result;
    }
    
    if (trailer->OPEN_PAREN()) {
        std::string funcName;
        
        // Get function name from the atom
        if (ctx->atom()->NAME()) {
            funcName = ctx->atom()->NAME()->getText();
        } else {
            funcName = result.toString();
            if (result.type == ValueType::STRING) {
                funcName = result.strVal;
            }
        }
        
        std::vector<Value> args;
        std::map<std::string, Value> kwargs;
        
        if (trailer->arglist()) {
            auto arguments = trailer->arglist()->argument();
            for (size_t i = 0; i < arguments.size(); i++) {
                auto arg = arguments[i];
                auto tests = arg->test();
                
                if (tests.size() == 2) {
                    std::string paramName = tests[0]->getText();
                    Value paramVal = std::any_cast<Value>(visit(tests[1]));
                    kwargs[paramName] = paramVal;
                } else if (tests.size() == 1) {
                    args.push_back(std::any_cast<Value>(visit(tests[0])));
                }
            }
        }
        
        result = callFunction(funcName, args, kwargs);
    }
    
    return result;
}

std::any EvalVisitor::visitTrailer(Python3Parser::TrailerContext *ctx) {
    return Value();
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (ctx->NAME()) {
        std::string name = ctx->NAME()->getText();
        
        if (name == "True") return Value(true);
        if (name == "False") return Value(false);
        if (name == "None") return Value();
        
        return getVariable(name);
    }
    
    if (ctx->NUMBER()) {
        std::string numStr = ctx->NUMBER()->getText();
        
        if (numStr.find('.') != std::string::npos) {
            return Value(std::stod(numStr));
        } else {
            return Value(BigInt(numStr));
        }
    }
    
    if (!ctx->STRING().empty()) {
        std::string result = "";
        
        auto strings = ctx->STRING();
        for (size_t i = 0; i < strings.size(); i++) {
            std::string s = strings[i]->getText();
            
            bool isFormatString = (s[0] == 'f' || s[0] == 'F');
            if (isFormatString) {
                s = s.substr(1);
            }
            
            char quote = s[0];
            s = s.substr(1, s.length() - 2);
            
            if (isFormatString) {
                s = processFormatString(s);
            }
            
            result += s;
        }
        
        return Value(result);
    }
    
    if (ctx->OPEN_PAREN() && ctx->test()) {
        return visit(ctx->test());
    }
    
    if (ctx->OPEN_PAREN()) {
        return Value(std::vector<Value>());
    }
    
    if (ctx->TRUE()) return Value(true);
    if (ctx->FALSE()) return Value(false);
    if (ctx->NONE()) return Value();
    
    if (ctx->format_string()) {
        return visit(ctx->format_string());
    }
    
    return Value();
}

std::any EvalVisitor::visitFormat_string(Python3Parser::Format_stringContext *ctx) {
    std::string result = "";
    
    auto literals = ctx->FORMAT_STRING_LITERAL();
    auto testlists = ctx->testlist();
    
    size_t litIdx = 0;
    size_t testIdx = 0;
    
    // Iterate through children to maintain order
    auto children = ctx->children;
    for (size_t i = 0; i < children.size(); i++) {
        auto child = children[i];
        
        // Check if it's a terminal node
        if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
            std::string text = terminal->getText();
            
            // Skip format quotation and regular quotation
            if (text == "f\"" || text == "F\"" || text == "f'" || text == "F'" ||
                text == "\"" || text == "'") {
                continue;
            }
            
            // Skip braces
            if (text == "{" || text == "}") {
                continue;
            }
            
            // This is a FORMAT_STRING_LITERAL
            result += text;
        } 
        // Check if it's a testlist node
        else if (dynamic_cast<Python3Parser::TestlistContext*>(child)) {
            if (testIdx < testlists.size()) {
                auto tests = testlists[testIdx]->test();
                if (tests.size() == 1) {
                    Value val = std::any_cast<Value>(visit(tests[0]));
                    result += val.toString();
                }
                testIdx++;
            }
        }
    }
    
    return Value(result);
}

std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    auto tests = ctx->test();
    
    if (tests.size() == 1) {
        return visit(tests[0]);
    }
    
    std::vector<Value> values;
    for (auto test : tests) {
        values.push_back(std::any_cast<Value>(visit(test)));
    }
    
    return Value(values);
}

std::any EvalVisitor::visitArglist(Python3Parser::ArglistContext *ctx) {
    return Value();
}

std::any EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx) {
    return Value();
}

std::any EvalVisitor::visitParameters(Python3Parser::ParametersContext *ctx) {
    return Value();
}

std::any EvalVisitor::visitTypedargslist(Python3Parser::TypedargslistContext *ctx) {
    return Value();
}

std::any EvalVisitor::visitAugassign(Python3Parser::AugassignContext *ctx) {
    return Value();
}
