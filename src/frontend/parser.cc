#include "parser.h"

#include <initializer_list>
#include <iostream>

namespace cilly {

Parser::Parser(std::vector<Token> tokens) :
    tokens_(std::move(tokens)), current_(0) {};

bool Parser::IsAtEnd() const {
  if (current_ >= static_cast<int>(tokens_.size())) {
    return true;
  }
  return tokens_[current_].kind == TokenKind::kEof;
}

const Token& Parser::Peek() const {
  return tokens_[current_];
}

const Token& Parser::Previous() const {
  assert(current_ > 0 && "没有更前面的 Token 了!");
  return tokens_[current_ - 1];
}

const Token& Parser::Advance() {
  if (!IsAtEnd()) {
    ++current_;
    return Previous();
  }
  return Previous();
}

const Token& Parser::LookAhead(int offset) const {
  if (current_ + offset >= static_cast<int>(tokens_.size())) {
    return tokens_.back();
  }
  return tokens_[current_ + offset];
}

bool Parser::IsAssignmentAhead() const {
  return Check(TokenKind::kIdentifier) &&
         LookAhead(1).kind == TokenKind::kEqual;
}

bool Parser::IsIndexAssignAhead() const {
  if (!Check(TokenKind::kIdentifier))
    return false;
  if (LookAhead(1).kind != TokenKind::kLBracket)
    return false;

  int i = 1;  // 从 '[' 开始
  int depth = 0;
  while (true) {
    TokenKind k = LookAhead(i).kind;
    if (k == TokenKind::kEof)
      return false;

    if (k == TokenKind::kLBracket)
      depth++;
    else if (k == TokenKind::kRBracket) {
      depth--;
      if (depth == 0) {
        // 这个 i 指向匹配的 ']'
        return LookAhead(i + 1).kind == TokenKind::kEqual;
      }
    }
    i++;
  }
}

bool Parser::IsPropAssignAhead() const {
  return (Check(TokenKind::kIdentifier) || Check(TokenKind::kThis)) &&
         LookAhead(1).kind == TokenKind::kDot &&
         LookAhead(2).kind == TokenKind::kIdentifier &&
         LookAhead(3).kind == TokenKind::kEqual;
}

bool Parser::Check(TokenKind kind) const {
  if (IsAtEnd())
    return false;
  return Peek().kind == kind;
}

bool Parser::Match(TokenKind kind) {
  if (Check(kind)) {
    Advance();
    return true;
  } else {
    return false;
  }
}

bool Parser::MatchAny(std::initializer_list<TokenKind> kinds) {
  for (auto i : kinds) {
    if (Check(i)) {
      Advance();
      return true;
    }
  }
  return false;
}

std::string Parser::StripQuotes(const std::string& s) {
  assert(s.size() >= 2 && s.front() == '"' && s.back() == '"');
  return s.substr(1, s.size() - 2);
}

StmtPtr Parser::VarDeclaration() {
  Token name = Consume(TokenKind::kIdentifier, "Expect variable name.");
  ExprPtr initializer = nullptr;
  if (Match(TokenKind::kEqual)) {
    initializer = Expression();  // 下一个步骤才实现，这一步用占位
  }
  Consume(TokenKind::kSemicolon, "Expect ';' after variable declaration.");
  return std::make_unique<VarStmt>(name, std::move(initializer));
}

StmtPtr Parser::ClassDeclaration() {
  Token name = Consume(TokenKind::kIdentifier, "Expect class name.");

  std::optional<Token> superclass;
  // 继承
  if (Match(TokenKind::kColon)) {
    superclass =
        Consume(TokenKind::kIdentifier, "Expect superclass name after ':'.");
  }

  Consume(TokenKind::kLBrace, "Expect '{' before class body.");

  std::vector<StmtPtr> methods_;
  while (!Check(TokenKind::kRBrace) && !IsAtEnd()) {
    Consume(TokenKind::kFun, "Expect 'fun' in class body (stage2).");
    StmtPtr func = FuncitonDeclaration();

    auto fn = static_cast<FunctionStmt*>(func.get());
    methods_.emplace_back(std::move(func));
  }
  Consume(TokenKind::kRBrace, "Expect '}' after class body.");
  return std::make_unique<ClassStmt>(name, std::move(superclass),
                                     std::move(methods_));
}

StmtPtr Parser::FuncitonDeclaration() {
  Token name = Consume(TokenKind::kIdentifier, "Expect function name.");
  std::cerr << "[debug] after fun name = " << name.lexeme
            << ", next kind = " << static_cast<int>(Peek().kind)
            << ", next lexeme = " << Peek().lexeme << "\n";
  Consume(TokenKind::kLParen, "Expect '(' after function name.");

  std::vector<Token> params;
  while (!Check(TokenKind::kRParen)) {
    Token name_params =
        Consume(TokenKind::kIdentifier, "Expect parameter name!");
    params.emplace_back(name_params);
    if (Check(TokenKind::kRParen))
      break;
    Consume(TokenKind::kComma, "Expect ',' after parameter name!");
  }
  Consume(TokenKind::kRParen, "Expect ')' after parameters.");
  // 规定函数体必须加‘{’
  Consume(TokenKind::kLBrace, "Expect '{' before function body.");

  StmtPtr body_stmt = BlockStatement();
  auto body =
      std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(body_stmt.release()));
  return std::make_unique<FunctionStmt>(name, params, std::move(body));
}

StmtPtr Parser::Statement() {
  if (Match(TokenKind::kLBrace)) {
    return BlockStatement();
  }
  if (Match(TokenKind::kPrint)) {
    return PrintStatement();
  }
  if (Match(TokenKind::kWhile)) {
    return WhileStatement();
  }
  if (Match(TokenKind::kFor)) {
    return ForStatement();
  }
  if (Match(TokenKind::kBreak)) {
    return BreakStatement();
  }
  if (Match(TokenKind::kContinue)) {
    return ContinueStatement();
  }
  if (Match(TokenKind::kIf)) {
    return IfStatement();
  }
  if (IsIndexAssignAhead()) {
    return IndexAssignStatement();
  }
  if (Check(TokenKind::kIdentifier) && LookAhead(1).kind == TokenKind::kEqual) {
    return AssignStatement();
  }
  if (Match(TokenKind::kReturn)) {
    return ReturnStatement();
  }
  if (IsPropAssignAhead()) {
    return PropAssignStatement();
  }
  return ExprStatement();
}

const Token& Parser::Consume(TokenKind kind, const std::string& message) {
  if (Check(kind)) {
    return Advance();
  } else {
    std::cerr
        << "Error: " << message
        << std::
               endl;  // std::cerr 是专门用来输出错误信息的流，它与 std::cout
                      // 是分开的，通常会在终端显示不同的颜色或标记，使得错误信息更加显眼。
    assert(false && message.c_str());  // 用c_str转换成const char*
                                       // 类型，主要可以转换成bool使用 而
                                       // string类型不可以，所以一定要用c_str
    return tokens_[current_];
  }
}

ExprPtr Parser::Expression() {
  return Equality();
}

// 取负
ExprPtr Parser::Unary() {
  if (Match(TokenKind::kMinus) || Match(TokenKind::kNot)) {
    Token op = Previous();
    ExprPtr expr = Unary();
    return std::make_unique<UnaryExpr>(op, std::move(expr));
  } else {
    return ProFix();
  }
}

// 乘除
ExprPtr Parser::Factor() {
  ExprPtr left = Unary();
  while (Match(TokenKind::kStar) || Match(TokenKind::kSlash)) {
    Token op = Previous();
    ExprPtr right = Unary();
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
  }
  return left;
}

// 加减
ExprPtr Parser::Term() {
  ExprPtr left = Factor();
  while (Match(TokenKind::kPlus) || Match(TokenKind::kMinus)) {
    Token op = Previous();
    ExprPtr right = Factor();
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
  }
  return left;
}

ExprPtr Parser::Comparison() {
  ExprPtr left = Term();
  while (Match(TokenKind::kLess) || Match(TokenKind::kGreater)) {
    Token op = Previous();
    ExprPtr right = Term();
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
  }
  return left;
}

ExprPtr Parser::Equality() {
  ExprPtr left = Comparison();
  while (Match(TokenKind::kEqualEqual) || Match(TokenKind::kNotEqual)) {
    Token op = Previous();
    ExprPtr right = Comparison();
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
  }
  return left;
}

ExprPtr Parser::Primary() {
  if (Match(TokenKind::kNumber)) {  // 数字
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kNumber,
                                         Previous().lexeme);
  } else if (Match(TokenKind::kIdentifier)) {  // 关键字
    return std::make_unique<VariableExpr>(Previous());
  } else if (Match(TokenKind::kString)) {
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kString,
                                         StripQuotes(Previous().lexeme));
  } else if (Match(TokenKind::kTrue)) {  // true
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kBool,
                                         Previous().lexeme);
  } else if (Match(TokenKind::kFalse)) {  // false
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kBool,
                                         Previous().lexeme);
  } else if (Match(TokenKind::kNull)) {  // null
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kNull,
                                         Previous().lexeme);
  } else if (Match(TokenKind::kLParen)) {  // "("
    ExprPtr value = Expression();
    Consume(TokenKind::kRParen, "Expect ')' after expression.");
    return value;
  } else if (Match(TokenKind::kLBracket)) {  // "["
    std::vector<ExprPtr> elements;
    while (!Match(TokenKind::kRBracket)) {
      elements.emplace_back(Expression());
      if (Match(TokenKind::kRBracket)) {
        break;
      }
      Consume(TokenKind::kComma, "Expect ',' after expression.");
    }
    return std::make_unique<ListExpr>(std::move(elements));
  } else if (Match(TokenKind::kLBrace)) {  // "{"
    std::vector<std::pair<std::string, ExprPtr>> entries;
    while (!Match(TokenKind::kRBrace)) {
      Token key = Consume(TokenKind::kString, "Dict key must be string.");
      std::string str;
      str = StripQuotes(key.lexeme);
      Consume(TokenKind::kColon, "Expect ':' after key.");
      ExprPtr value = Expression();
      entries.emplace_back(std::move(str), std::move(value));
      if (Match(TokenKind::kRBrace)) {
        break;
      }
      Consume(TokenKind::kComma, "Expect ',' after expression.");
    }
    return std::make_unique<DictExpr>(std::move(entries));
  } else if (Match(TokenKind::kThis)) {
    return std::make_unique<ThisExpr>(Previous());
  } else {
    assert(false && "未找到此种类型!");
    return nullptr;
  }
}

ExprPtr Parser::ProFix() {
  ExprPtr expr = Primary();

  while (true) {
    if (Match(TokenKind::kLBracket)) {
      ExprPtr idx = Expression();
      Consume(TokenKind::kRBracket, "Expect ']' after expression.");
      expr = std::make_unique<IndexExpr>(std::move(expr), std::move(idx));
      continue;
    }

    if (Match(TokenKind::kLParen)) {
      std::vector<ExprPtr> args;
      if (!Check(TokenKind::kRParen)) {
        do {
          if (args.size() >= 255) {
            assert(false && "Too many arguments (max 255).");
          }
          args.emplace_back(Expression());
        } while (Match(TokenKind::kComma));
      }
      Token paren = Consume(TokenKind::kRParen, "Expect ')' after arguments.");
      expr = std::make_unique<CallExpr>(std::move(expr), std::move(args),
                                        std::move(paren));
      continue;
    }

    if (Match(TokenKind::kDot)) {
      Token name =
          Consume(TokenKind::kIdentifier, "Expect property name after '.'.");
      expr = std::make_unique<GetPropExpr>(std::move(expr), name);
      continue;
    }

    break;
  }

  return expr;
}

StmtPtr Parser::PrintStatement() {
  ExprPtr value = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  return std::make_unique<PrintStmt>(std::move(value));
}

StmtPtr Parser::BlockStatement() {
  auto block = std::make_unique<BlockStmt>();
  while (!IsAtEnd() && !Check(TokenKind::kRBrace)) {
    block->statements.emplace_back(Declaration());
  }
  Consume(TokenKind::kRBrace, "Expect '}' after block.");
  return block;
}

StmtPtr Parser::AssignStatement() {
  return AssignStatement(true);
}

StmtPtr Parser::AssignStatement(bool require_semicolon) {
  Token name = Advance();
  Consume(TokenKind::kEqual, "Expect '=' after value.");
  ExprPtr expr = Expression();
  if (require_semicolon) {
    Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  }
  return std::make_unique<AssignStmt>(std::move(name), std::move(expr));
}

StmtPtr Parser::IndexAssignStatement() {
  Token name = Consume(TokenKind::kIdentifier, "Expect identifier.");
  ExprPtr object = std::make_unique<VariableExpr>(std::move(name));

  Consume(TokenKind::kLBracket, "Expect '[' after identifier.");
  ExprPtr index = Expression();
  Consume(TokenKind::kRBracket, "Expect ']' after index.");

  Consume(TokenKind::kEqual, "Expect '=' after ']'.");
  ExprPtr value = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after index assignment.");

  return std::make_unique<IndexAssignStmt>(std::move(object), std::move(index),
                                           std::move(value));
}

StmtPtr Parser::ReturnStatement() {
  ExprPtr expr = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after expr.");
  return std::make_unique<ReturnStmt>(std::move(expr));
}

StmtPtr Parser::PropAssignStatement() {
  ExprPtr obj_expr = nullptr;

  if (Match(TokenKind::kThis)) {
    Token t = Previous();
    obj_expr = std::make_unique<ThisExpr>(t);
  } else {
    Token obj = Consume(TokenKind::kIdentifier, "Expect object name.");
    obj_expr = std::make_unique<VariableExpr>(std::move(obj));
  }

  Consume(TokenKind::kDot, "Expect '.' after expr.");
  Token name = Consume(TokenKind::kIdentifier, "Expect property name.");
  Consume(TokenKind::kEqual, "Expect '=' after property name.");
  ExprPtr expr = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after assignment.");

  return std::make_unique<PropAssignStmt>(std::move(obj_expr), name,
                                          std::move(expr));
}

StmtPtr Parser::ExprStatement() {
  ExprPtr value = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  return std::make_unique<ExprStmt>(std::move(value));
}

StmtPtr Parser::WhileStatement() {
  Consume(TokenKind::kLParen, "Expect '(' after while.");
  ExprPtr cond = Expression();
  Consume(TokenKind::kRParen, "Expect ')' after cond.");
  StmtPtr body = Statement();  // 这里也是，不能直接去确定是块语句
  return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

// for
StmtPtr Parser::ForStatement() {
  // 拆分
  Consume(TokenKind::kLParen, "Expect '(' after for.");
  StmtPtr init = nullptr;
  if (!Check(TokenKind::kSemicolon)) {
    if (Match(TokenKind::kVar)) {
      init = VarDeclaration();
    } else {
      init = AssignStatement();
    }
  } else {
    Consume(TokenKind::kSemicolon, "Expect ';' after init.");
  }

  ExprPtr cond = nullptr;
  if (!Check(TokenKind::kSemicolon)) {
    cond = Expression();
  }
  Consume(TokenKind::kSemicolon, "Expect ';' after cond.");

  StmtPtr step = nullptr;
  if (!Check(TokenKind::kRParen)) {
    if (IsAssignmentAhead()) {
      step = AssignStatement(false);
    } else {
      step = std::make_unique<ExprStmt>(std::move(Expression()));
    }
  }
  Consume(TokenKind::kRParen, "Expect ')' after step.");

  StmtPtr body;
  body =
      Statement();  // 可以是单句，不一定是块语句，所以不直接等于BlockStatment

  if (!cond) {
    cond =
        std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kBool, "true");
  }

  return std::make_unique<ForStmt>(std::move(init), std::move(cond),
                                   std::move(step), std::move(body));
}

StmtPtr Parser::BreakStatement() {
  Consume(TokenKind::kSemicolon, "Expect ';' after break.");
  return std::make_unique<BreakStmt>();
}

StmtPtr Parser::ContinueStatement() {
  Consume(TokenKind::kSemicolon, "Expect ';' after continue.");
  return std::make_unique<ContinueStmt>();
}

// if
StmtPtr Parser::IfStatement() {
  Consume(TokenKind::kLParen, "Expect '(' after if.");
  ExprPtr cond = Expression();
  Consume(TokenKind::kRParen, "Expect ')' after condition.");

  StmtPtr then_branch_ = Statement();

  StmtPtr else_branch_ = nullptr;
  if (Match(TokenKind::kElse)) {
    if (Match(TokenKind::kIf)) {
      else_branch_ = IfStatement();
    } else
      else_branch_ = Statement();
  }
  return std::make_unique<IfStmt>(std::move(cond), std::move(then_branch_),
                                  std::move(else_branch_));
}

// 主要调用函数
std::vector<StmtPtr> Parser::ParseProgram() {
  std::vector<StmtPtr> result;
  while (!IsAtEnd()) {
    StmtPtr stmt = Declaration();
    result.push_back(std::move(stmt));
  }
  return result;
}

StmtPtr Parser::Declaration() {
  if (Match(TokenKind::kVar)) {
    return VarDeclaration();
  }
  if (Match(TokenKind::kFun)) {
    assert(block_depth_ == 0 && "暂时不实现非顶层声明!");
    return FuncitonDeclaration();
  }
  if (Match(TokenKind::kClass)) {
    assert(block_depth_ == 0 && "class must be top-level for now!");
    return ClassDeclaration();
  }
  return Statement();
}

}  // namespace cilly
