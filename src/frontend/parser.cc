#include <initializer_list>
#include <iostream>
#include "parser.h"

namespace cilly {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), current_(0) {};

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
  if(!IsAtEnd()) {
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

bool Parser::Check(TokenKind kind) const {
  if(IsAtEnd()) return false;
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
    if(Check(i)) {
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
    initializer = Expression();   // 下一个步骤才实现，这一步用占位
  }
  Consume(TokenKind::kSemicolon, "Expect ';' after variable declaration.");
  return std::make_unique<VarStmt>(name, std::move(initializer));
}

StmtPtr Parser::Statement() {
  if (Match(TokenKind::kPrint)) {   // 打印语句
    return PrintStatement();
  } else if (Match(TokenKind::kLBrace)) {  // 块语句
    return BlockStatement();
  } else if (Check(TokenKind::kIdentifier) && LookAhead(1).kind == TokenKind::kEqual) {
    return AssignStatement();
  } else {  
    return ExprStatement();
  }
}

const Token& Parser::Consume(TokenKind kind, const std::string& message) {
  if (Check(kind)) {
    return Advance();
  } else {
    std::cerr << "Error: " << message << std::endl; //std::cerr 是专门用来输出错误信息的流，它与 std::cout 是分开的，通常会在终端显示不同的颜色或标记，使得错误信息更加显眼。
    assert(false && message.c_str());  // 用c_str转换成const char* 类型，主要可以转换成bool使用 而 string类型不可以，所以一定要用c_str
    return tokens_[current_]; 
  }
}

ExprPtr Parser::Expression() {
  return Term();
}

// 取负
ExprPtr Parser::Unary() {   
  if (Match(TokenKind::kMinus)) {
    Token op = Previous();
    ExprPtr right = Unary();
    ExprPtr zero = std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kNumber, "0");
    return std::make_unique<BinaryExpr> (std::move(zero), op, std::move(right));
  } else {
    return Primary();
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



ExprPtr Parser::Primary() {
  if (Match(TokenKind::kNumber)) {  // 数字
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kNumber,Previous().lexeme);
  } 
  else if (Match(TokenKind::kIdentifier)) {  // 关键字
    return std::make_unique<VariableExpr>(Previous());
  } 
  else if (Match(TokenKind::kTrue)) {                        // true
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kBool,Previous().lexeme);
  } 
  else if (Match(TokenKind::kFalse)) {                        // false
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kBool,Previous().lexeme);
  } 
  else if (Match(TokenKind::kNull)) {                        // null
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kNull,Previous().lexeme);
  } 
  else if (Match(TokenKind::kLParen)) {      // "("
    ExprPtr value = Expression();
    Consume(TokenKind::kRParen, "Expect ')' after expression.");
    return value;
  }
  else if (Match(TokenKind::kLBracket)) {    // "["
    std::vector<ExprPtr> elements;
    while (!Match(TokenKind::kRBracket)) {
      elements.emplace_back(Expression());
      if (Match(TokenKind::kRBracket)) {
        break;
      }
      Consume(TokenKind::kComma, "Expect ',' after expression.");
    }
    return std::make_unique<ListExpr>(std::move(elements));
  } 
  else if (Match(TokenKind::kLBrace)) {     // "{"
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
  }
    else {
    assert(false && "未找到此种类型!");
    return nullptr;
  }
}

StmtPtr Parser::PrintStatement() {
  ExprPtr value = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  return std::make_unique<PrintStmt>(std::move(value));
}

// 未完全定义
StmtPtr Parser::BlockStatement() {
  // 暂时先返回一个空 BlockStmt
  auto block = std::make_unique<BlockStmt>();
  
  // 未来这里会解析语句直到遇到 }
  // 现在我们只是为了不报错
  Consume(TokenKind::kRBrace, "Expect '}' after block.");
  return block;
}

StmtPtr Parser::AssignStatement() {
  Token name;
  ExprPtr expr;
  name = Advance();
  Consume(TokenKind::kEqual, "Expect '=' after value.");
  expr = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  return std::make_unique<AssignStmt>(std::move(name), std::move(expr));
}

StmtPtr Parser::ExprStatement() {
  ExprPtr value = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  return std::make_unique<ExprStmt>(std::move(value));
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
  if(Match(TokenKind::kVar)) {
    return VarDeclaration();
  } else {
    return Statement();
  }
}

}  // namespace cilly
