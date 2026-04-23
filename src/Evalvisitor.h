#pragma once
#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H

#include "Python3ParserBaseVisitor.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

class BigInt {
private:
    std::vector<int> digits;
    bool negative;
    
    void removeLeadingZeros();
    
public:
    BigInt();
    BigInt(long long n);
    BigInt(const std::string& s);
    
    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;
    BigInt operator*(const BigInt& other) const;
    BigInt operator/(const BigInt& other) const;
    BigInt operator%(const BigInt& other) const;
    BigInt operator-() const;
    
    bool operator<(const BigInt& other) const;
    bool operator>(const BigInt& other) const;
    bool operator<=(const BigInt& other) const;
    bool operator>=(const BigInt& other) const;
    bool operator==(const BigInt& other) const;
    bool operator!=(const BigInt& other) const;
    
    std::string toString() const;
    double toDouble() const;
    bool isZero() const;
    bool isNegative() const;
    
    BigInt abs() const;
};

enum class ValueType {
    NONE,
    BOOL,
    INT,
    FLOAT,
    STRING,
    TUPLE,
    FUNCTION
};

struct Function;

class Value {
public:
    ValueType type;
    bool boolVal;
    BigInt intVal;
    double floatVal;
    std::string strVal;
    std::vector<Value> tupleVal;
    std::shared_ptr<Function> funcVal;
    
    Value();
    Value(bool b);
    Value(const BigInt& i);
    Value(double f);
    Value(const std::string& s);
    Value(const std::vector<Value>& t);
    
    std::string toString() const;
    bool toBool() const;
    BigInt toInt() const;
    double toFloat() const;
    
    Value operator+(const Value& other) const;
    Value operator-(const Value& other) const;
    Value operator*(const Value& other) const;
    Value operator/(const Value& other) const;
    Value operator%(const Value& other) const;
    Value operator-() const;
    
    bool operator<(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>=(const Value& other) const;
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
};

struct Function {
    std::string name;
    std::vector<std::string> params;
    std::vector<Value> defaults;
    Python3Parser::SuiteContext* body;
    std::map<std::string, Value> capturedScope;
};

enum class FlowControl {
    NONE,
    BREAK,
    CONTINUE,
    RETURN
};

class EvalVisitor : public Python3ParserBaseVisitor {
private:
    std::vector<std::map<std::string, Value>> scopes;
    std::map<std::string, Value> globalScope;
    FlowControl flowControl;
    Value returnValue;
    
    void enterScope();
    void exitScope();
    Value getVariable(const std::string& name);
    void setVariable(const std::string& name, const Value& value);
    bool hasVariable(const std::string& name);
    
    Value callFunction(const std::string& name, const std::vector<Value>& args, 
                      const std::map<std::string, Value>& kwargs);
    Value builtinPrint(const std::vector<Value>& args);
    Value builtinInt(const Value& arg);
    Value builtinFloat(const Value& arg);
    Value builtinStr(const Value& arg);
    Value builtinBool(const Value& arg);
    
    std::string processFormatString(const std::string& fstr);
    
public:
    EvalVisitor();
    
    std::any visitFile_input(Python3Parser::File_inputContext *ctx) override;
    std::any visitFuncdef(Python3Parser::FuncdefContext *ctx) override;
    std::any visitParameters(Python3Parser::ParametersContext *ctx) override;
    std::any visitTypedargslist(Python3Parser::TypedargslistContext *ctx) override;
    std::any visitStmt(Python3Parser::StmtContext *ctx) override;
    std::any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override;
    std::any visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) override;
    std::any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override;
    std::any visitAugassign(Python3Parser::AugassignContext *ctx) override;
    std::any visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) override;
    std::any visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) override;
    std::any visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) override;
    std::any visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) override;
    std::any visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) override;
    std::any visitIf_stmt(Python3Parser::If_stmtContext *ctx) override;
    std::any visitWhile_stmt(Python3Parser::While_stmtContext *ctx) override;
    std::any visitSuite(Python3Parser::SuiteContext *ctx) override;
    std::any visitTest(Python3Parser::TestContext *ctx) override;
    std::any visitOr_test(Python3Parser::Or_testContext *ctx) override;
    std::any visitAnd_test(Python3Parser::And_testContext *ctx) override;
    std::any visitNot_test(Python3Parser::Not_testContext *ctx) override;
    std::any visitComparison(Python3Parser::ComparisonContext *ctx) override;
    std::any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override;
    std::any visitTerm(Python3Parser::TermContext *ctx) override;
    std::any visitFactor(Python3Parser::FactorContext *ctx) override;
    std::any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override;
    std::any visitTrailer(Python3Parser::TrailerContext *ctx) override;
    std::any visitAtom(Python3Parser::AtomContext *ctx) override;
    std::any visitTestlist(Python3Parser::TestlistContext *ctx) override;
    std::any visitArglist(Python3Parser::ArglistContext *ctx) override;
    std::any visitArgument(Python3Parser::ArgumentContext *ctx) override;
};

#endif//PYTHON_INTERPRETER_EVALVISITOR_H
