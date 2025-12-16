#include "lexer.h"

namespace cilly {

// 构造函数：只是把传进来的源码存起来，然后把游标都初始化到开头。
Lexer::Lexer(std::string source)
    : source_(std::move(source)),
      current_(0),
      start_(0),
      line_(1),
      col_(1),
      token_start_col_(1),
      token_start_line_(1) {}

bool Lexer::IsAtEnd() const {
  return current_ >= static_cast<int>(source_.size());
}

char Lexer::Peek() const {
  return IsAtEnd() ? '\0' : source_[current_];
}

char Lexer::PeekNext() const {
  return current_ + 1 >= static_cast<int>(source_.size()) ? '\0' : source_[current_ + 1];
}

char Lexer::Advance() {
  char pre_char = Peek();
  ++current_;
  ++col_;
  if (pre_char == '\n') {
    ++line_;
    col_ = 1;
  }
  return pre_char;
}

bool Lexer::Match(char expected) {
  if (IsAtEnd()) return false;
  if (source_[current_] != expected) return false;
  ++current_;
  ++col_;
  return true;
}

void Lexer::SkipWhitespace() {
  while(true) {
    char ch = Peek();
    if (ch == ' ' || ch == '\r' || ch == '\t') {
      Advance();
      continue;
    } else if (ch == '\n') {
      Advance();
      continue;
    } else if (ch == '/' && PeekNext() == '/') {
      while (ch != '\n' && !IsAtEnd()) {
        ch = Peek();
        Advance();
      }
      continue;
    } else {
      break;
    }
  }
}

bool Lexer::IsAlpha(char c) {
  return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_';
}

bool Lexer::IsDigit(char c) {
  return c >= '0' && c <= '9';
}

bool Lexer::IsString(char c) {
  return c == '"';
}

bool Lexer::IsAlphaNumeric(char c) {
  return IsAlpha(c) || IsDigit(c);
}



Token Lexer::ScanIdentifier() {
  std::string s;
  char ch = Peek();
  while (IsAlphaNumeric(ch)) {
    Advance();
    ch = Peek();
  }
  s = std::move(source_.substr(start_, current_ - start_));
  if (s == "var") { return MakeToken(TokenKind::kVar); }
  else if (s == "if") { return MakeToken(TokenKind::kIf); }
  else if (s == "else") { return MakeToken(TokenKind::kElse); }
  else if (s == "for") { return MakeToken(TokenKind::kFor); }
  else if (s == "while") { return MakeToken(TokenKind::kWhile); }
  else if (s == "print") { return MakeToken(TokenKind::kPrint); }
  else if (s == "true") { return MakeToken(TokenKind::kTrue); }
  else if (s == "false") { return MakeToken(TokenKind::kFalse); }
  else if (s == "null") { return MakeToken(TokenKind::kNull); }
  else return MakeToken(TokenKind::kIdentifier);
}



Token Lexer::ScanNumber() {
  char ch = Peek();
  while (IsDigit(ch)) {
    Advance();
    ch = Peek();
  }
  return MakeToken(TokenKind::kNumber);
}

Token Lexer::ScanString() {
  while (!IsAtEnd() && Peek() != '\n' && Peek() != '"') {   // 暂时禁止跨行
    Advance();
  }
  if (IsAtEnd()) {
      assert(false && "字符串未闭合!");
  }
  if (Peek() == '\n') {
      assert(false && "字符串禁止跨行!");
  }
  Advance();
  return MakeToken(TokenKind::kString);
}



Token Lexer::MakeToken(TokenKind kind) {
  Token token;
  token.kind = kind;
  token.lexeme = std::move(source_.substr(start_, current_ - start_));
  token.col = token_start_col_;
  token.line = token_start_line_;
  return token;
}



Token Lexer::ScanToken() {
  char ch = Advance();
  if (IsAlpha(ch)) {
    return ScanIdentifier();
  } else if (IsDigit(ch)) {
    return ScanNumber();
  } else if (IsString(ch)) {
    return ScanString();
  } else{
      switch (ch){
        case '(' : 
          return MakeToken(TokenKind::kLParen);
        case ')' :
          return MakeToken(TokenKind::kRParen);
        case '{' :
          return MakeToken(TokenKind::kLBrace);
        case '}' : 
          return MakeToken(TokenKind::kRBrace);
        case ',' : 
          return MakeToken(TokenKind::kComma);
        case ';' :
          return MakeToken(TokenKind::kSemicolon);
        case '+' :
          return MakeToken(TokenKind::kPlus);
        case '-' :
          return MakeToken(TokenKind::kMinus);
        case '*' :
          return MakeToken(TokenKind::kStar);
        case '/' :
          return MakeToken(TokenKind::kSlash);
        case '<' :
          return MakeToken(TokenKind::kLess);
        case '=': {
          if (PeekNext() == '=') {
            return MakeToken(TokenKind::kEqualEqual);
          } else {
            return MakeToken(TokenKind::kEqual);
          }
        }
        case '[' :
          return MakeToken(TokenKind::kLBracket);
        case ']' :
          return MakeToken(TokenKind::kRBracket);
        case ':' :
          return MakeToken(TokenKind::kColon);
        default :
          assert(false && "还无法识别其他字符！");
      }
  }
}



// 现在的简易版：先什么都不切，只返回一个 EOF token，
// 目的是让整个流程先能跑起来，后面再往这里加真正的扫描逻辑。
std::vector<Token> Lexer::ScanAll() {
  std::vector<Token> tokens;
  while (!IsAtEnd()) {
    Token token;
    SkipWhitespace();
    if(IsAtEnd()) break;
    start_ = current_;
    token_start_col_ = col_;
    token_start_line_ = line_;
    token = std::move(ScanToken());
    tokens.emplace_back(token);
  }
  Token eof;
  eof.kind = TokenKind::kEof;
  eof.col = col_;
  eof.lexeme = "";
  eof.line = line_;
  tokens.emplace_back(eof);
  
  return tokens;
}

}  // namespace cilly
