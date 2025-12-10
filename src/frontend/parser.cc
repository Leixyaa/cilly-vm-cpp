#include "parser.h"

namespace cilly {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), current_(0) {};

bool Parser::IsAtEnd() const {
  return current_ == static_cast<int>(tokens_.size());
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
  return Peek();
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

const Token& Parser::Consume(TokenKind kind, const std::string& message) {
  if (Check(kind)) {
    return Advance();
  } else {
    assert(false && message.c_str());  // 用c_str转换成const char* 类型，主要可以转换成bool使用 而 string类型不可以，所以一定要用c_str
  }
}

std::vector<StmtPtr> Parser::ParseProgram() {
  std::vector<StmtPtr> result;
  return result;
}

}  // namespace cilly
