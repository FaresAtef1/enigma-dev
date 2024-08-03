/** Copyright (C) 2024 Fares Atef
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include <JDI/src/System/builtins.h>
#include "ast.h"

using namespace enigma::parsing;

#define VISIT_AND_CHECK(node) \
  if (!Visit(node)) return false;

bool AST::Visitor::VisitIdentifierAccess(IdentifierAccess &node) {
  if (print_type) print("auto ");
  print(std::string(node.name.content));
  return true;
}

bool AST::Visitor::VisitLiteral(Literal &node) {
  std::string value = std::get<std::string>(node.value.value);
  if (node.value.type != TT_CHARLIT && node.value.type != TT_STRINGLIT) {
    print(value);
    return true;
  }

  print(node.value.type == TT_CHARLIT ? "'" : "\"");
  std::string to_print;
  for (char c : value) {
    if (c == '\\') {
      to_print += "\\\\";
    } else if (c >= ' ' && c <= '~') {
      to_print += c;
    } else if (c == '\n') {
      to_print += "\\n";
    } else if (c == '\t') {
      to_print += "\\t";
    } else if (c == '\v') {
      to_print += "\\v";
    } else if (c == '\b') {
      to_print += "\\b";
    } else if (c == '\r') {
      to_print += "\\r";
    } else if (c == '\f') {
      to_print += "\\f";
    } else if (c == '\a') {
      to_print += "\\a";
    } else if (c == '\?') {
      to_print += "\\?";
    } else {
      std::ostringstream oss;
      oss << '\\' << std::oct << static_cast<int>(c);
      to_print += oss.str();
    }
  }
  print(to_print);
  print(node.value.type == TT_CHARLIT ? "'" : "\"");

  return true;
}

bool AST::Visitor::VisitParenthetical(Parenthetical &node) {
  print("(");
  if (node.expression) {
    VISIT_AND_CHECK(node.expression);
  }
  print(")");
  return true;
}

bool AST::Visitor::VisitUnaryPostfixExpression(UnaryPostfixExpression &node) {
  VISIT_AND_CHECK(node.operand);
  print(node.operation.token);
  return true;
}

bool AST::Visitor::VisitUnaryPrefixExpression(UnaryPrefixExpression &node) {
  print(node.operation.token);
  if (node.operation.type == TT_STAR && node.operand->type != AST::NodeType::PARENTHETICAL) {
    print("(");
  }

  VISIT_AND_CHECK(node.operand);

  if (node.operation.type == TT_STAR && node.operand->type != AST::NodeType::PARENTHETICAL) {
    print(")");
  }
  return true;
}

bool AST::Visitor::VisitDeleteExpression(DeleteExpression &node) {
  if (node.is_global) {
    print("::");
  }
  print("delete ");
  if (node.is_array) {
    print("[] ");
  }

  VISIT_AND_CHECK(node.expression);

  return true;
}

bool AST::Visitor::VisitBreakStatement(BreakStatement &node) {
  print("break");
  if (node.count) {
    print(" ");
    VISIT_AND_CHECK(node.count);
  }
  return true;
}

bool AST::Visitor::VisitContinueStatement(ContinueStatement &node) {
  print("continue");
  if (node.count) {
    print(" ");
    VISIT_AND_CHECK(node.count);
  }
  return true;
}

bool AST::Visitor::VisitWithStatement(WithStatement &node) {
  print("with");
  if (node.object->type != AST::NodeType::PARENTHETICAL) {
    print("(");
  }
  VISIT_AND_CHECK(node.object);
  if (node.object->type != AST::NodeType::PARENTHETICAL) {
    print(")");
  }

  VISIT_AND_CHECK(node.body);
  PrintSemiColon(node.body);

  return true;
}

bool AST::Visitor::VisitBinaryExpression(BinaryExpression &node) {
  VISIT_AND_CHECK(node.left);

  print(" " + node.operation.token + " ");

  VISIT_AND_CHECK(node.right);

  if (node.operation.type == TT_BEGINBRACKET) {
    print("]");
  }
  return true;
}

bool AST::Visitor::VisitFunctionCallExpression(FunctionCallExpression &node) {
  VISIT_AND_CHECK(node.function);
  print("(");

  for (auto &arg : node.arguments) {
    VISIT_AND_CHECK(arg);
    if (&arg != &node.arguments.back()) {
      print(", ");
    }
  }

  print(")");
  return true;
}

bool AST::Visitor::VisitTernaryExpression(TernaryExpression &node) {
  VISIT_AND_CHECK(node.condition);
  print(" ? ");

  VISIT_AND_CHECK(node.true_expression);
  print(" : ");

  VISIT_AND_CHECK(node.false_expression);
  return true;
}

bool AST::Visitor::VisitLambdaExpression(LambdaExpression &node) {
  print("[&]");

  if (node.parameters->type == AST::NodeType::IDENTIFIER) {
    print("(");
  }
  print_type = true;
  VISIT_AND_CHECK(node.parameters);
  print_type = false;
  if (node.parameters->type == AST::NodeType::IDENTIFIER) {
    print(")");
  }

  if (node.body->type != AST::NodeType::BLOCK) {
    print("{");
  }
  VISIT_AND_CHECK(node.body);
  PrintSemiColon(node.body);
  if (node.body->type != AST::NodeType::BLOCK) {
    print("}");
  }

  return true;
}

bool AST::Visitor::VisitReturnStatement(ReturnStatement &node) {
  print("return ");
  if (node.expression) {
    VISIT_AND_CHECK(node.expression);
  }
  return true;
}

bool AST::Visitor::VisitFullType(FullType &ft, bool print_type) {
  if (print_type) {
    std::vector<std::size_t> flags_values = {jdi::builtin_flag__const->value,    jdi::builtin_flag__static->value,
                                             jdi::builtin_flag__volatile->value, jdi::builtin_flag__mutable->value,
                                             jdi::builtin_flag__register->value, jdi::builtin_flag__inline->value,
                                             jdi::builtin_flag__Complex->value,  jdi::builtin_flag__unsigned->value,
                                             jdi::builtin_flag__signed->value,   jdi::builtin_flag__short->value,
                                             jdi::builtin_flag__long->value,     jdi::builtin_flag__long_long->value,
                                             jdi::builtin_flag__restrict->value, jdi::builtin_typeflag__override->value,
                                             jdi::builtin_typeflag__final->value};

    std::vector<std::size_t> flags_masks = {
        jdi::builtin_flag__const->mask,    jdi::builtin_flag__static->mask,       jdi::builtin_flag__volatile->mask,
        jdi::builtin_flag__mutable->mask,  jdi::builtin_flag__register->mask,     jdi::builtin_flag__inline->mask,
        jdi::builtin_flag__Complex->mask,  jdi::builtin_flag__unsigned->mask,     jdi::builtin_flag__signed->mask,
        jdi::builtin_flag__short->mask,    jdi::builtin_flag__long->mask,         jdi::builtin_flag__long_long->mask,
        jdi::builtin_flag__restrict->mask, jdi::builtin_typeflag__override->mask, jdi::builtin_typeflag__final->mask};

    std::vector<std::string> flags_names = {"const",  "static",    "volatile", "mutable",  "register",
                                            "inline", "complex",   "unsigned", "signed",   "short",
                                            "long",   "long long", "restrict", "override", "final"};

    for (std::size_t i = 0; i < flags_values.size(); i++) {
      if ((ft.flags & flags_masks[i]) == flags_values[i]) {
        if (flags_names[i] != "signed" || (flags_names[i] == "signed" && ft.def->name == "char")) {
          print(flags_names[i] + " ");
        }
      }
    }

    print(ft.def->name + " ");
  }

  std::string name = std::string(ft.decl.name.content);
  if (name != "" && !ft.decl.components.size()) {
    print(name + " ");
  }

  jdi::ref_stack stack;
  ft.decl.to_jdi_refstack(stack);
  auto first = stack.begin();

  std::string ref;
  bool flag = false;
  bool print_name = true;

  for (auto it = first; it != stack.end(); it++) {
    if (it->type == jdi::ref_stack::RT_POINTERTO) {
      flag = true;
      ref = '*' + ref;
    } else if (it->type == jdi::ref_stack::RT_REFERENCE) {
      flag = true;
      ref = '&' + ref;
    } else {
      if (it->type == jdi::ref_stack::RT_ARRAYBOUND) {
        if (flag) {
          ref = '(' + ref + ')';
        }

        std::size_t arr_size = it->arraysize();
        if (arr_size != 0) {
          ref += '[' + std::to_string(arr_size) + ']';
        } else {
          ref += "[]";
        }
      } else {
        print("RT_MEMBER_POINTER");
      }

      // TODO: RT_MEMBER_POINTER

      flag = false;
    }

    if (print_name) {
      std::string name = std::string(ft.decl.name.content);
      if (name != "") {
        if (it->type == jdi::ref_stack::RT_ARRAYBOUND) {
          ref = name + ref;
        } else {
          ref += name;
        }
      }
      print_name = false;
    }
  }

  print(ref);
  return true;
}

bool AST::Visitor::VisitSizeofExpression(SizeofExpression &node) {
  print("sizeof");

  if (node.kind == AST::SizeofExpression::Kind::EXPR) {
    print(" ");
    auto &arg = std::get<AST::PNode>(node.argument);
    VISIT_AND_CHECK(arg);
  } else if (node.kind == AST::SizeofExpression::Kind::VARIADIC) {
    print("...(");
    std::string arg = std::get<std::string>(node.argument);
    print(arg + ")");
  } else {
    print("(");

    FullType &ft = std::get<FullType>(node.argument);
    if (!VisitFullType(ft)) return false;

    print(")");
  }

  return true;
}

bool AST::Visitor::VisitAlignofExpression(AlignofExpression &node) {
  print("alignof(");
  if (!VisitFullType(node.ft)) return false;
  print(")");
  return true;
}

bool AST::Visitor::VisitCastExpression(CastExpression &node) {
  if (node.kind == AST::CastExpression::Kind::FUNCTIONAL) {
    if (!VisitFullType(node.ft)) return false;
    print("(");
  } else if (node.kind == AST::CastExpression::Kind::C_STYLE) {
    print("(");
    if (!VisitFullType(node.ft)) return false;
    print(")");
  } else {
    if (node.kind == AST::CastExpression::Kind::STATIC) {
      print("static_cast<");
    } else if (node.kind == AST::CastExpression::Kind::DYNAMIC) {
      print("dynamic_cast<");
    } else if (node.kind == AST::CastExpression::Kind::CONST) {
      print("const_cast<");
    } else if (node.kind == AST::CastExpression::Kind::REINTERPRET) {
      print("reinterpret_cast<");
    }
    if (!VisitFullType(node.ft)) return false;
    print(">(");
  }

  if (node.expr) {
    VISIT_AND_CHECK(node.expr);
  }

  if (node.kind == AST::CastExpression::Kind::FUNCTIONAL) {
    print(")");
  } else if (node.kind != AST::CastExpression::Kind::C_STYLE) {
    print(")");
  }

  return true;
}

bool AST::Visitor::VisitArray(Array &node) {
  print("[");
  if (node.elements.size()) {
    VISIT_AND_CHECK(node.elements[0]);
  }
  print("]");
  return true;
}

bool AST::Visitor::VisitBraceOrParenInitializer(BraceOrParenInitializer &node) {
  if (node.kind == BraceOrParenInitializer::Kind::PAREN_INIT) {
    print("(");
  } else {
    print("{");
  }

  for (auto &val : node.values) {
    if (node.kind == BraceOrParenInitializer::Kind::DESIGNATED_INIT) {
      print(".");
    }

    if (val.first != "") {
      print(val.first + "=");
    }

    if (!VisitInitializer(*val.second)) return false;

    if (&val != &node.values.back()) {
      print(", ");
    }
  }

  if (node.kind == BraceOrParenInitializer::Kind::PAREN_INIT) {
    print(")");
  } else {
    print("}");
  }

  return true;
}

bool AST::Visitor::VisitAssignmentInitializer(AssignmentInitializer &node) {
  if (node.kind == AssignmentInitializer::Kind::BRACE_INIT) {
    if (!VisitBraceOrParenInitializer(*std::get<BraceOrParenInitNode>(node.initializer))) return false;
  } else {
    auto &expr = std::get<AST::PNode>(node.initializer);
    VISIT_AND_CHECK(expr);
  }
  return true;
}

bool AST::Visitor::VisitInitializer(Initializer &node) {
  if (node.kind == Initializer::Kind::BRACE_INIT || node.kind == Initializer::Kind::PLACEMENT_NEW) {
    auto &init = std::get<BraceOrParenInitNode>(node.initializer);
    if (!VisitBraceOrParenInitializer(*init)) return false;
  } else if (node.kind == Initializer::Kind::ASSIGN_EXPR) {
    auto &init = std::get<AssignmentInitNode>(node.initializer);
    if (!VisitAssignmentInitializer(*init)) return false;
  }

  if (node.is_variadic) {
    print("...");
  }

  return true;
}

bool AST::Visitor::VisitNewExpression(NewExpression &node) {
  if (node.is_global) {
    print("::");
  }

  print("new ");

  if (node.placement) {
    if (!VisitInitializer(*node.placement)) return false;
    print(" ");
  }

  print("(");
  if (!VisitFullType(node.ft)) return false;
  print(")");

  if (node.initializer) {
    if (!VisitInitializer(*node.initializer)) return false;
  }

  return true;
}

bool AST::Visitor::VisitDeclarationStatement(DeclarationStatement &node) {
  for (std::size_t i = 0; i < node.declarations.size(); i++) {
    if (!VisitFullType(*node.declarations[i].declarator, !i)) return false;
    if (node.declarations[i].init) {
      print(" = ");  // TODO: corner case: int x {}, maybe we need extra flag in the AST?
      if (!VisitInitializer(*node.declarations[i].init)) return false;
    }
    if (i != node.declarations.size() - 1) {
      print(", ");
    }
  }
  return true;
}

bool AST::Visitor::VisitCode(CodeBlock &node) {
  for (auto &stmt : node.statements) {
    VISIT_AND_CHECK(stmt);
    PrintSemiColon(stmt);
  }
  return true;
}

bool AST::Visitor::VisitCodeBlock(CodeBlock &node) {
  print("{");
  if (!VisitCode(node)) return false;
  print("}");
  return true;
}

bool AST::Visitor::VisitIfStatement(IfStatement &node) {
  print("if");
  if (node.condition->type != AST::NodeType::PARENTHETICAL) {
    print("(");
  }

  VISIT_AND_CHECK(node.condition);

  if (node.condition->type != AST::NodeType::PARENTHETICAL) {
    print(")");
  }

  print(" ");
  if (node.true_branch) {
    VISIT_AND_CHECK(node.true_branch);
    PrintSemiColon(node.true_branch);
  } else {
    print(";");
  }
  print(" ");

  if (node.false_branch) {
    print("else ");
    VISIT_AND_CHECK(node.false_branch);
    PrintSemiColon(node.false_branch);
    print(" ");
  }

  return true;
}

bool AST::Visitor::VisitForLoop(ForLoop &node) {
  print("for(");

  VISIT_AND_CHECK(node.assignment);
  print("; ");

  VISIT_AND_CHECK(node.condition);
  print("; ");

  VISIT_AND_CHECK(node.increment);
  print(") ");

  VISIT_AND_CHECK(node.body);
  PrintSemiColon(node.body);
  print(" ");

  return true;
}

bool AST::Visitor::VisitCaseStatement(CaseStatement &node) {
  print("case ");
  VISIT_AND_CHECK(node.value);

  print(": ");
  if (!VisitCodeBlock(*node.statements->As<AST::CodeBlock>())) return false;
  print(" ");

  return true;
}

bool AST::Visitor::VisitDefaultStatement(DefaultStatement &node) {
  print("default: ");
  if (!VisitCodeBlock(*node.statements->As<CodeBlock>())) return false;
  print(" ");
  return true;
}

bool AST::Visitor::VisitSwitchStatement(SwitchStatement &node) {
  print("switch");
  if (node.expression->type != AST::NodeType::PARENTHETICAL) {
    print("(");
  }

  VISIT_AND_CHECK(node.expression);

  if (node.expression->type != AST::NodeType::PARENTHETICAL) {
    print(")");
  }
  print(" ");

  if (!VisitCodeBlock(*node.body->As<CodeBlock>())) return false;
  print(" ");

  return true;
}

bool AST::Visitor::VisitWhileLoop(WhileLoop &node) {
  // temp sol
  if (node.kind == AST::WhileLoop::Kind::REPEAT) {
    print("int strange_name = ");
  } else {
    print("while");
    if (node.condition->type != AST::NodeType::PARENTHETICAL) {
      print("(");
    }

    if (node.kind == AST::WhileLoop::Kind::UNTIL) {
      if (node.condition->type == AST::NodeType::PARENTHETICAL) {
        print("(!");
      } else {
        print("!(");
      }
    }
  }

  VISIT_AND_CHECK(node.condition);

  if (node.kind != AST::WhileLoop::Kind::REPEAT) {
    if (node.kind == AST::WhileLoop::Kind::UNTIL) {
      print(")");
    }

    if (node.condition->type != AST::NodeType::PARENTHETICAL) {
      print(")");
    }
  } else {
    print("; while(strange_name--)");
  }

  print(" ");
  VISIT_AND_CHECK(node.body);
  PrintSemiColon(node.body);

  return true;
}

bool AST::Visitor::VisitDoLoop(DoLoop &node) {
  print("do");

  if (node.body->type != AST::NodeType::BLOCK) {
    print("{");
  }

  VISIT_AND_CHECK(node.body);
  PrintSemiColon(node.body);

  if (node.body->type != AST::NodeType::BLOCK) {
    print("}");
  }

  print("while");
  if (node.condition->type != AST::NodeType::PARENTHETICAL) {
    print("(");
  }

  if (node.is_until) {
    if (node.condition->type == AST::NodeType::PARENTHETICAL) {
      print("(!");
    } else {
      print("!(");
    }
  }

  VISIT_AND_CHECK(node.condition);

  if (node.is_until) {
    print(")");
  }
  if (node.condition->type != AST::NodeType::PARENTHETICAL) {
    print(")");
  }
  print(";");

  return true;
}
