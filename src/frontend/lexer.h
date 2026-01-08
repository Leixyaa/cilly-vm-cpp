#ifndef CILLY_VM_CPP_LEXER_H_
#define CILLY_VM_CPP_LEXER_H_

#include <assert.h>

#include <string>
#include <vector>

namespace cilly {

//  TokenKind：碰到的“单词类型”的分类
enum class TokenKind {
  // 文件结束（End Of File）
  kEof,

  // 基本类型
  kIdentifier,  // 标识符：变量名/函数名，比如 foo, x
  kNumber,      // 数字：123, 3.14
  kString,      // 字符串："hello"

  // 关键字
  kVar,       // var
  kFun,       // fun
  kIf,        // if
  kElse,      // else
  kFor,       // for
  kWhile,     // while
  kPrint,     // print
  kTrue,      // true
  kFalse,     // false
  kNull,      // null
  kBreak,     // break
  kContinue,  // continue;
  kReturn,    // return
  kClass,     // class
  kThis,      // this
  kSuper,     // super

  // 符号/分隔符
  kLParen,     // (
  kRParen,     // )
  kLBrace,     // {
  kRBrace,     // }
  kComma,      // ,
  kSemicolon,  // ;
  kLBracket,   // [
  kRBracket,   // ]
  kColon,      // :
  kDot,        // .

  // 运算符
  kPlus,        // +
  kMinus,       // -
  kStar,        // *
  kSlash,       // /
  kEqual,       // =
  kEqualEqual,  // ==
  kLess,        // <
  kGreater,     // >
  kNot,         // !
  kNotEqual,    // !=
};

// Token：一个单词的具体信息。
struct Token {
  TokenKind kind;      // 判断类型
  std::string lexeme;  // 源代码里的原始文本
  int line;            // 出现在第几行（从 1 开始）
  int col;             // 出现在这一行的第几列（从 1 开始）
};

// Lexer 类：负责拿到整段代码字符串，然后切成一串 Token
class Lexer {
 public:
  // 构造函数：把源码字符串丢进来存起来。
  explicit Lexer(std::string source);

  // 主功能：把整段源码切成 token 序列。
  std::vector<Token> ScanAll();

 private:
  // 判断：是否已经到达字符串末尾
  bool IsAtEnd() const;

  // 看当前字符，但不前进
  char Peek() const;

  // 看下一个字符，但不前进
  char PeekNext() const;

  // 读一个字符，并把 current_ / line_ / col_ 往前推
  char Advance();

  // 如果下一个字符等于 expected，就“吃掉它”，返回 true；
  // 否则什么也不动，返回 false。
  bool Match(char expected);

  // 跳过空白和注释（空格、制表符、回车、换行、// 注释）
  void SkipWhitespace();

  // 判断一个字符是不是字母或下划线（适合作为标识符的第一个字符）
  static bool IsAlpha(char c);

  // 判断是不是数字
  static bool IsDigit(char c);

  // 判断是不是字符串
  static bool IsString(char c);

  // 判断是不是字母 / 下划线 / 数字（适合作为标识符的后续字符）
  static bool IsAlphaNumeric(char c);

  // 扫描一个标识符或关键字，从 start_ 开始一路吃到“不是标识符字符”为止
  Token ScanIdentifier();

  Token ScanNumber();

  Token ScanString();

  // 构造一个 Token（后面会复用）
  Token MakeToken(TokenKind kind);

  // 扫描一个 token（假定前面已经 SkipWhitespace，且当前不是 EOF）
  Token ScanToken();

  // 整个源码文本
  std::string source_;
  // current_：当前读到的字符位置（下标）
  int current_;
  // start_：当前这个 token 的开始位置（下标）
  int start_;
  // 行号，从 1 开始
  int line_;
  // 当前行里的列号，从 1 开始
  int col_;

  int token_start_line_;
  int token_start_col_;
};

}  // namespace cilly

#endif  // CILLY_VM_CPP_LEXER_H_
