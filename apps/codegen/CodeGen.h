/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019
 *
 *  @file  CodeGen.h
 *  @brief A system to generate dynamic concept code for C++17.
 *  @note Status: PLANNING
 *
 *  NOTES:
 *   - TYPE_OR_EXPRESSION: Collect everything until you hit an unmatched close-mark: ')', ']', '}', or '>'
 *   - STATEMENT: Collect everything until you hit a ';' outside of parens.
 *   - STATEMENT_LIST: (nothing)                  (an unmatched close-mark next requires this option)
 *                   | STATEMENT STATEMENT_LIST
 *   - BLOCK: '{' STATEMENT_LIST '}'
 * 
 *   - TYPE: ID TYPE_END
 *   - TYPE_END: (nothing)
 *             | "::" TYPE
 *             | "<" TYPE_OR_EXPRESSION ">" TYPE_END
 *   - DECLARE: TYPE ID
 *   - FUNCTION: DECLARE '(' PARAMS ')'
 *   - PARAMS: (nothing)
 *           | PARAM_LIST
 *   - PARAM_LIST: PARAM
 *               | PARAM ',' PARAM_LIST
 *   - PARAM: DECLARE
 *          | OVERLOAD '(' ID ')'
 *   - MEMBER: DECLARE ';'
 *           | FUNCTION BLOCK
 *           | "using" ID '=' TYPE ';'
 *           | "using" ID '=' "REQUIRE" '(' STRING ')' ';'
 */

#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "../../source/base/Ptr.h"
#include "../../source/tools/Lexer.h"

class CodeGen {
private:
  std::string filename;
  emp::Lexer lexer;
  emp::vector<emp::Token> tokens;
  bool debug = false;

  int token_identifier = -1;
  int token_number = -1;
  int token_string = -1;
  int token_other = -1;

  // All AST Nodes have a common base class.
  struct AST_Node {
    emp::vector<emp::Ptr<AST_Node>> children;
    ~AST_Node() { for (auto x : children) x.Delete(); }
    void AddChild(emp::Ptr<AST_Node> node_ptr) { children.push_back(node_ptr); }
  };

  // Misc. Code that should just be echoed back out.
  struct AST_Code : AST_Node {
    std::string code;
  };

  struct AST_Block : AST_Node {
    // Children are a series of statements.
  };

  struct AST_Using : AST_Node {
    std::string type_name;
    std::string type_value;
  };

  struct AST_VarDeclare : AST_Node {
    std::string var_name;
    // Child is an AST_Code expression.
  };

  // Full concept information.
  struct AST_Concept : AST_Node {
    std::string name;
    std::string base_name;
    // Children are Using, Variable Declaration, or Function Declaration
  };

  struct AST_ConceptUsing : AST_Node {
    std::string type_name;
    std::string default_code;
  };

  struct AST_ConceptVariable : AST_Node {
    std::string var_type;
    std::string var_name;
    std::string default_code;
  };

  struct AST_ConceptFunction : AST_Node {
    std::string return_type;
    std::string fun_name;
    std::string args;
    std::set<std::string> attributes;     // const, noexcept, etc.
    std::string default_code;
    bool is_required = false;
    bool is_default = false;

    std::string AttributeString() {
      std::string out_str;
      for (auto & x : attributes) out_str += x;
      return out_str;
    }
  };

  AST_Node ast_root;


  // Helper functions
  bool HasToken(int pos) const { return (pos >= 0) && (pos < tokens.size()); }
  bool IsID(int pos) const { return HasToken(pos) && tokens[pos].token_id == token_identifier; }
  bool IsNumber(int pos) const { return HasToken(pos) && tokens[pos].token_id == token_number; }
  bool IsString(int pos) const { return HasToken(pos) && tokens[pos].token_id == token_string; }
  char AsChar(int pos) const {
    if (HasToken(pos) && tokens[pos].token_id == token_other) return tokens[pos].lexeme[0];
    return 0;
  }
  const std::string & AsLexeme(int pos) const {
    return HasToken(pos) ? tokens[pos].lexeme : emp::empty_string();
  }

  std::string ConcatLexemes(size_t start_pos, size_t end_pos) {
    emp_assert(start_pos <= end_pos);
    emp_assert(end_pos <= tokens.size());
    std::stringstream ss;    
    for (size_t i = start_pos; i < end_pos; i++) {
      if (i > start_pos) ss << " ";
      ss << tokens[i].lexeme;
    }
    return ss.str();
  }

  void Error(const std::string & msg, int pos = -1) {
    std::cout << "Error (token " << pos << "): " << msg << "\nAborting." << std::endl;
    exit(1);
  }

  template <typename... Ts>
  void Debug(Ts... args) {
    if (debug) std::cout << "DEBUG: " << emp::to_string(args...) << std::endl;
  }

  void RequireID(int pos, const std::string & error_msg) {
    if (!IsID(pos)) { Error(error_msg, pos); }
  }
  void RequireNumber(int pos, const std::string & error_msg) {
    if (!IsNumber(pos)) { Error(error_msg, pos); }
  }
  void RequireString(int pos, const std::string & error_msg) {
    if (!IsString(pos)) { Error(error_msg, pos); }
  }
  void RequireChar(char req_char, int pos, const std::string & error_msg) {
    if (AsChar(pos) != req_char) { Error(error_msg, pos); }
  }
  void RequireLexeme(const std::string & req_str, int pos, const std::string & error_msg) {
    if (AsLexeme(pos) != req_str) { Error(error_msg, pos); }
  }

public:
  CodeGen(std::string in_filename) : filename(in_filename) {
    // Whitespace and comments should always be dismissed (top priority)
    lexer.AddToken("Whitespace", "[ \t\n\r]+", false, false);                // Any form of whitespace.
    lexer.AddToken("Comment", "//.*", true, false);                          // Any '//'-style comment.

    // Meaningful tokens have next priority.
    token_identifier = lexer.AddToken("ID", "[a-zA-Z_][a-zA-Z0-9_]+", true, true);   // Identifiers
    token_number = lexer.AddToken("Number", "[0-9]+(.[0-9]+)?", true, true); // Literal numbers.
    token_string = lexer.AddToken("String", "\\\"[^\"]*\\\"", true, true);   // Literal strings.

    // Other tokens should have least priority.
    token_other = lexer.AddToken("Other", ".|\"::\"", true, true);           // Symbols

    std::ifstream file(filename);
    tokens = lexer.Tokenize(file);
    file.close();
  }

  // Collect a line of code, ending with a semi-colon OR mis-matched bracket.
  // Always stops at a mis-matched ')' '}' or ']'
  // If match_angle_bracket is set, will also stop at a mis-matched '>'
  // If multi_line is set, will NOT stop with a ';'
  size_t ProcessCode(size_t pos, std::string & line, bool match_angle_bracket=false, bool multi_line=false) {
    const size_t start_pos = pos;
    std::vector<char> open_symbols;
    bool finished = false;
    while (!finished && pos < tokens.size()) {
      char cur_char = AsChar(pos++);
      switch (cur_char) {
        case ';':
          if (multi_line == false) finished = true;
          break;
        case '<':
          if (match_angle_bracket == false) break;
          [[fallthrough]]
        case '(':
        case '[':
        case '{':
          open_symbols.push_back(cur_char);  // Store this open bracket to be matched later.
          break;
        case '>':
          if (match_angle_bracket == false) break;
          [[fallthrough]]
        case ')':
        case ']':
        case '}':
          if (open_symbols.size()) {
            // @CAO should check to make sure this is a CORRECT match...
            open_symbols.pop_back();
            break;
          }
          // We will only make it here is this is an unmatched bracket.
          pos--;              // Leave close bracket to still be processed.
          finished = true;
          break;
      }
    }

    line = ConcatLexemes(start_pos, pos);

    return pos;
  }

  // Collect all tokens used to describe a type.
  size_t ProcessType(size_t pos, std::string & type_name) {
    const size_t start_pos = pos;
    // A type may start with a const.
    if (AsLexeme(pos) == "const") pos++;

    // Figure out the identifier (with possible "::" requiring another id)
    bool need_id = true;
    while (need_id) {
      if (AsLexeme(pos) == "typename") pos++;  // May specify a typename is next.
      if (AsLexeme(pos) == "template") pos++;  // May specify a template is next.

      RequireID(pos, emp::to_string("Expecting type, but found '", tokens[pos].lexeme, "'."));
      pos++;
      need_id = false;

      // In case this is a template, we need to evaluate parameters.
      if (AsLexeme(pos) == "<") {
        pos = ProcessCode(pos+1, type_name, true);
        RequireChar('>', pos, "Templates must end in a close angle bracket.");
      }

      if (AsLexeme(pos) == "::") {
        pos++;
        need_id = true;
      }
    }

    // Type may end in a symbol...
    if (AsLexeme(pos) == "&") pos++;
    if (AsLexeme(pos) == "*") pos++;

    // Collect all of the lexemes
    type_name = ConcatLexemes(start_pos, pos);

    return pos;
  }

  // Collect a series of identifiers, separated by spaces.
  size_t ProcessIDList(size_t pos, std::set<std::string> & ids) {
    while (IsID(pos)) {
      ids.insert(AsLexeme(pos));
      pos++;
    }
    return pos;
  }

 
  // Process the tokens starting from the outer-most scope.
  size_t ProcessTop(size_t pos=0) {
    while (pos < tokens.size()) {
      RequireID(pos, "Statements in outer scope must begi with an identifier or keyword.");

      if (tokens[pos].lexeme == "concept") {
        auto node_ptr = emp::NewPtr<AST_Concept>();
        ast_root.AddChild(node_ptr);
        pos = ProcessConcept(pos + 1, *node_ptr);
      }
      // @CAO: Technically we can have a whole list of special keywords, but for now its just "concept".
      else {
        Error( emp::to_string("Unknown keyword '", tokens[pos].lexeme, "'.  Aborting."), pos );
      }
    }
    return pos;
  }

  // We know we are in a concept definition.  Collect appropriate information.
  size_t ProcessConcept(size_t pos, AST_Concept & concept) {
    // A concept must begin with its name.
    RequireID(pos, "Concept declaration must be followed by name identifier.");
    concept.name = tokens[pos++].lexeme;

    // Next, must be a colon...
    RequireChar(':', pos, "Concept names must be followed by a colon (':').");
    pos++;

    // And then a base-class name.
    RequireID(pos, "Concept declaration must include name of base class.");
    concept.base_name = tokens[pos++].lexeme;

    Debug("Defining concept '", concept.name, "' with base class '", concept.base_name, "'.");

    // Next, must be an open brace...
    RequireChar('{', pos, "Concepts must be defined in braces ('{' and '}').");
    pos++;

    // Loop through the full definition of concept, incorporating each entry.
    while ( AsChar(pos) != '}' ) {
      // Entries can be a "using" statement, a function definition, or a variable definition.
      RequireID(pos, "Concept members can be either functions, variables, or using-statements.");

      if (tokens[pos].lexeme == "using") {              // ----- USING!! -----
        pos++;  // Move past "using"
        RequireID(pos, "A 'using' command must first specify the new type name.");

        auto node_using = emp::NewPtr<AST_ConceptUsing>();  // Setup an AST node for a using statement.       
        concept.AddChild(node_using);                       // Save this node in the concept.
        pos = ProcessType(pos, node_using->type_name);      // Determine new type name being defined.

        Debug("...adding a type '", node_using->type_name, "'.");

        RequireChar('=', pos++, "A using statement must provide an equals ('=') to assign the type.");

        pos = ProcessCode(pos, node_using->default_code);   // Determine code being assigned to.

        Debug("   value: ", node_using->default_code);
      } else {
        // Start with a type...
        std::string type_name;
        pos = ProcessType(pos, type_name);

        // Then an identifier.
        RequireID(pos, "Functions and variables in concept definition must provide identifier after type name.");
        std::string identifier = tokens[pos++].lexeme;

        // If and open-paren follows the identifier, we are defining a function, otherwise it's a variable.
        if (AsChar(pos) == '(') {                                // ----- FUNCTION!! -----
          pos++;  // Move past paren.

          // Setup an AST Node for a function definition.
          auto node_function = emp::NewPtr<AST_ConceptFunction>();
          node_function->return_type = type_name;
          node_function->fun_name = identifier;
          concept.AddChild(node_function);                       // Save this function node in the concept.


          pos = ProcessCode(pos, node_function->args);           // Read the args for this function.

          RequireChar(')', pos++, "Function arguments must end with a close-parenthesis (')')");

          Debug("...adding a function '", type_name, " ", identifier, "(", node_function->args, ")'");

          pos = ProcessIDList(pos, node_function->attributes);   // Read in each of the function attributes, if any.

          Debug("   with attributes: ", node_function->AttributeString());

          char fun_char = AsChar(pos++);

          if (fun_char == '=') {  // Function is "= default;" or "= required;"
            RequireID(pos, "Function must be assigned to 'required' or 'default'");
            std::string fun_assign = AsLexeme(pos++);
            if (fun_assign == "required") node_function->is_required = true;
            else if (fun_assign == "default") node_function->is_default = true;
            else Error("Functions can only be set to 'required' or 'default'", pos);
            RequireChar(';', pos++, emp::to_string(fun_assign, "functions must end in a semi-colon."));
          }
          else if (fun_char == '{') {  // Function is defined in place.
            pos = ProcessCode(pos, node_function->default_code, false, true);  // Read the default function body.

            Debug("   and code: ", node_function->default_code);

            RequireChar('}', pos++, emp::to_string("Function body must end with close brace ('}') not '",
                                                  AsLexeme(pos-1), "'."));
          }
          else {
            Error("Function body must begin with open brace or assignment ('{' or '=')", pos-1);
          }

        } else {                                                 // ----- VARIABLE!! -----
          auto node_var = emp::NewPtr<AST_ConceptVariable>();
          node_var->var_type = type_name;
          node_var->var_name = identifier;

          if (AsChar(pos) == ';') {  // Does the variable declaration end here?
            pos++;
          }
          else {                     // ...or is there a default value for this variable?
            // Determine code being assigned from.
            pos = ProcessCode(pos, node_var->default_code);
          }

        }
      }
    }

    pos++;  // Skip closing brace.
    RequireChar(';', pos++, "Concept definitions must end in a semi-colon.");

    return pos;
  }
  
  void PrintLexerState() { lexer.Print(); }

  void PrintTokens() {
    for (size_t pos = 0; pos < tokens.size(); pos++) {
      std::cout << pos << ": "
                << lexer.GetTokenName(tokens[pos])
                << " : \"" << AsLexeme(pos) << "\""
                << std::endl;
    }
    // for (auto token : tokens) {
    //   std::cout << lexer.GetTokenName(token) << " : \"" << token.lexeme << "\"" << std::endl;
    // }
  }

  void SetDebug(bool in_debug=true) { debug = in_debug; }

};