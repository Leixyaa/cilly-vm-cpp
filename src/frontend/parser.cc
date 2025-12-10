#include <initializer_list>
#include "parser.h"

namespace cilly {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), current_(0) {};

bool Parser::IsAtEnd() const {
  return current_ >= static_cast<int>(tokens_.size());
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
  } else {  
    return ExprStatement();
  }
}

const Token& Parser::Consume(TokenKind kind, const std::string& message) {
  if (Check(kind)) {
    return Advance();
  } else {
    assert(false && message.c_str());  // 用c_str转换成const char* 类型，主要可以转换成bool使用 而 string类型不可以，所以一定要用c_str
    return tokens_[current_]; 
  }
}

ExprPtr Parser::Expression() {
  return Primary();
}

ExprPtr Parser::Primary() {
  if (Match(TokenKind::kNumber)) {
    return std::make_unique<LiteralExpr>(LiteralExpr::LiteralKind::kNumber,Previous().lexeme);
  } else if (Match(TokenKind::kIdentifier)) {
    return std::make_unique<VariableExpr>(Previous());
  } else if (Match(TokenKind::kLParen)) {
    ExprPtr value = Expression();
    Consume(TokenKind::kRParen, "Expect ')' after expression.");
    return value;
  } else {
    assert(false && "未找到此种类型!");
    return nullptr;
  }
}

StmtPtr Parser::PrintStatement() {
  ExprPtr value = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  return std::make_unique<PrintStmt>(std::move(value));
}

StmtPtr Parser::BlockStatement() {
  // 暂时先返回一个空 BlockStmt
  auto block = std::make_unique<BlockStmt>();
  
  // 未来这里会解析语句直到遇到 }
  // 现在我们只是为了不报错
  Consume(TokenKind::kRBrace, "Expect '}' after block.");
  return block;
}

StmtPtr Parser::ExprStatement() {
  ExprPtr value = Expression();
  Consume(TokenKind::kSemicolon, "Expect ';' after value.");
  return std::make_unique<ExprStmt>(std::move(value));
}

}  // namespace cilly
