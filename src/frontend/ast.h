#ifndef CILLY_VM_CPP_AST_H_
#define CILLY_VM_CPP_AST_H_

#include <memory>
#include <string>
#include <vector>
#include <initializer_list>

#include "lexer.h"  // 为了用 Token 这个类型

namespace cilly {

// 前向声明，后面用别名简化。
struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;





// ======================
// 表达式（Expression）
// ======================

struct Expr {
  enum class Kind {
    kLiteral,   // 字面量：数字、true、false、null
    kVariable,  // 变量引用：x
    kBinary,    // 二元表达式：a + b
    kList,
    kDict,
    kIndex,
    // 后面还会加：一元表达式、赋值表达式、函数调用等等
  };

  explicit Expr(Kind kind) : kind(kind) {}
  virtual ~Expr() = default;

  Kind kind;
};



// 字面量类型：数字 / 布尔 / null
struct LiteralExpr : public Expr {
  enum class LiteralKind {
    kNumber,
    kBool,
    kNull,
    kString,
  };

  LiteralExpr(LiteralKind literal_kind, std::string lexeme)
      : Expr(Kind::kLiteral),
        literal_kind(literal_kind),
        lexeme(std::move(lexeme)) {}

  LiteralKind literal_kind;
  // 源码里的文本，比如 "123" / "true" / "null"
  std::string lexeme;
};





// 变量引用：例如表达式里出现 x、y_1 这种
struct VariableExpr : public Expr {
  explicit VariableExpr(Token name)
      : Expr(Kind::kVariable), name(std::move(name)) {}

  // 记录变量名对应的 token，方便以后报错（行号、列号）
  Token name;
};





// 二元表达式：left op right，例如 1 + 2 或 x == 3
struct BinaryExpr : public Expr {
  BinaryExpr(ExprPtr left, Token op, ExprPtr right)
      : Expr(Kind::kBinary),
        left(std::move(left)),
        op(std::move(op)),
        right(std::move(right)) {}

  ExprPtr left;
  Token op;     // 运算符对应的 token（比如 +、-、==）
  ExprPtr right;
};



struct ListExpr : public Expr {
  ListExpr(std::vector<ExprPtr> elements_) : Expr(Kind::kList), elements(std::move(elements_)) {}
  std::vector<ExprPtr> elements;
};


struct DictExpr : public Expr {
  DictExpr(std::vector<std::pair<std::string, ExprPtr>> entries_)
      : Expr(Kind::kDict),
      entries(std::move(entries_)) {
  }
  std::vector<std::pair<std::string, ExprPtr>> entries;
};


struct IndexExpr : public Expr {
  IndexExpr(ExprPtr object_, ExprPtr expr_) 
      : Expr(Kind::kIndex),
      object(std::move(object_)),
      expr(std::move(expr_)) {
  }
  ExprPtr object;
  ExprPtr expr;
};



// ======================
// 语句（Statement）
// ======================

struct Stmt {
  enum class Kind {
    kVar,     // 变量声明：var x = expr;
    kExpr,    // 表达式语句：expr;
    kPrint,   // print 语句：print expr;
    kBlock,   // 复合语句：{ stmt* }
    kAssign,   // 赋值语句：x = expr;
    kIndexAssign, // 索引赋值语句 x[i] = expr;
    kWhile,
    kFor,
    kBreak,
    kContinue,
    kIf,
  };

  explicit Stmt(Kind kind) : kind(kind) {}
  virtual ~Stmt() = default;

  Kind kind;
};

// var 声明：var name = initializer;
struct VarStmt : public Stmt {
  VarStmt(Token name, ExprPtr initializer)
      : Stmt(Kind::kVar),
        name(std::move(name)),
        initializer(std::move(initializer)) {}

  Token name;
  // 允许 initializer 为空：var x; 这种情况（以后 Parser 控制）
  ExprPtr initializer;
};

// 表达式语句：expr;
struct ExprStmt : public Stmt {
  explicit ExprStmt(ExprPtr expr)
      : Stmt(Kind::kExpr), expr(std::move(expr)) {}

  ExprPtr expr;
};

// print 语句：print expr;
struct PrintStmt : public Stmt {
  explicit PrintStmt(ExprPtr expr)
      : Stmt(Kind::kPrint), expr(std::move(expr)) {}

  ExprPtr expr;
};

// while 语句
struct WhileStmt : public Stmt {
  WhileStmt(ExprPtr cond_, StmtPtr body_)
      : Stmt(Kind::kWhile),
        cond(std::move(cond_)),
        body(std::move(body_)){}
  ExprPtr cond;
  StmtPtr body;
};

// for 语句
struct ForStmt : public Stmt {
  ForStmt(StmtPtr init_, ExprPtr cond_, StmtPtr step_, StmtPtr body_)
      : Stmt(Kind::kFor),
        init(std::move(init_)),
        cond(std::move(cond_)),
        step(std::move(step_)),
        body(std::move(body_)) {}

  StmtPtr init;   
  ExprPtr cond;  
  StmtPtr step;  
  StmtPtr body;   
};

//break 语句
struct BreakStmt : public Stmt {
  BreakStmt() : Stmt(Kind::kBreak){}
};

// continue 语句
struct ContinueStmt : public Stmt {
  ContinueStmt() : Stmt(Kind::kContinue){}
};

// 代码块：{ stmt1; stmt2; ... }
struct BlockStmt : public Stmt {
  BlockStmt() : Stmt(Kind::kBlock) {}
  std::vector<StmtPtr> statements;
};

// 赋值语句
struct AssignStmt : public Stmt {
  AssignStmt(Token name, ExprPtr expr) 
      : Stmt(Kind::kAssign), 
        name(std::move(name)), 
        expr(std::move(expr)){}
  
  Token name;
  ExprPtr expr;
};

// 索引赋值语句
struct IndexAssignStmt : public Stmt {
  IndexAssignStmt(ExprPtr object_, ExprPtr index_, ExprPtr expr_) 
      : Stmt(Kind::kIndexAssign),
        object(std::move(object_)),
        index(std::move(index_)),
        expr(std::move(expr_)){}
  
  ExprPtr object;
  ExprPtr index;
  ExprPtr expr;
};

struct IfStmt : public Stmt {
  IfStmt(ExprPtr cond_, StmtPtr then_, StmtPtr else_)
      : Stmt(Kind::kIf),
      cond(std::move(cond_)),
        then_branch(std::move(then_)),
        else_branch(std::move(else_)){}

  ExprPtr cond;
  StmtPtr then_branch;
  StmtPtr else_branch;
};


}  // namespace cilly

#endif  // CILLY_VM_CPP_AST_H_
