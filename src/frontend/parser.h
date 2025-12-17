#ifndef CILLY_VM_CPP_PARSER_H_
#define CILLY_VM_CPP_PARSER_H_

#include <initializer_list>
#include <string>
#include <vector>

#include "ast.h"

namespace cilly {

// Parser 负责：
//   Token 序列  ->  语句列表（AST）
//
// 用法大致会是：
//   Lexer lexer(source);
//   auto tokens = lexer.ScanAll();
//   Parser parser(tokens);
//   auto program = parser.ParseProgram();  // program 是 vector<StmtPtr>

class Parser {
 public:
  explicit Parser(std::vector<Token> tokens);

  // 解析整个程序，返回一串语句（顶层都是 Stmt）
  std::vector<StmtPtr> ParseProgram();

 private:
  // 基础游标工具:

  // 当前是否已经到达 token 列表末尾（通常是遇到 kEof）
  bool IsAtEnd() const;

  // 返回当前 token（不会前进）
  const Token& Peek() const;

  // 返回上一个 token
  const Token& Previous() const;

  // 如果当前 token 的 kind 是期望的 kind，就消费掉并返回 true；
  // 否则不动，返回 false。
  bool Match(TokenKind kind);

  // 和上面类似，但是可以传入多个候选 kind，只要匹配任意一个就消费掉。
  bool MatchAny(std::initializer_list<TokenKind> kinds);

  // 除去string两边引号 "abc"->abc
  std::string StripQuotes(const std::string& s);  

  // 如果当前 token 是 kind，就消费掉；否则报错（以后再实现具体错误处理）。
  const Token& Consume(TokenKind kind, const std::string& message);

  // 前进一步：返回当前 token，然后 current_++
  const Token& Advance();

  const Token& LookAhead(int offset) const;

  bool IsAssignmentAhead() const;

  bool IsIndexAssignAhead() const;

  // 检查当前 token 的 kind 是否是给定 kind（不前进）
  bool Check(TokenKind kind) const;

  // 语法分析入口

  // 解析一条“声明语句”,例如 var 声明，或其他声明。
  StmtPtr Declaration();

  // 解析一条“普通语句”,例如表达式语句、print 语句、块语句等。
  StmtPtr Statement();

  // var 声明语句：var x = expr;
  StmtPtr VarDeclaration();

  // print 语句：print expr;
  StmtPtr PrintStatement();

  // 表达式语句：expr;
  StmtPtr ExprStatement();

  // while 语句; while() {}
  StmtPtr WhileStatement();

  // for 语句；降级为while
  StmtPtr ForStatement();

  // break 语句；当前只能用在while和for内
  StmtPtr BreakStatement();

  // continue语句;
  StmtPtr ContinueStatement();

  // If 语句；if {} else {}
  StmtPtr IfStatement();

  // 块语句：{ stmt* }
  StmtPtr BlockStatement();

  // 赋值语句：x = expr;
  StmtPtr AssignStatement();

  // 赋值语句（选择是否去掉分号）
  StmtPtr AssignStatement(bool require_semicolon); // 重载，应对赋值表达式情况

  // 索引赋值语句
  StmtPtr IndexAssignStatement();	

  // ========== 表达式语法 ==========

  // 总入口：表达式
  ExprPtr Expression();

  // 后面这几个我们会一步步实现：从简单到复杂
  // 你可以先只把名字写在这里，不用去管每一层具体含义。
  //
  // 典型表达式优先级结构（从高到低）大概是：
  //   Primary -> Unary -> Factor(* /) -> Term(+ -) -> ... -> Expression

  //ExprPtr Equality();    // ==, !=（以后用）
  //ExprPtr Comparison();  // <, <=, >, >=（以后用）
  ExprPtr Term();        // +, -
  ExprPtr Factor();      // *, /
  ExprPtr Unary();       // 一元运算：-expr, !expr（以后用）
  ExprPtr Primary();     // 最底层：字面量、变量名、括号表达式
  ExprPtr ProFix();
  ExprPtr Equality();
  ExprPtr Comparison();

  // ========== 成员数据 ==========

  std::vector<Token> tokens_;  // 输入的 token 序列
  int current_;                // 当前看的 token 下标（类似 Lexer 里的 current_）
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_PARSER_H_
