#include <stdio.h>
#include <assert.h>
#include "compiler.h"
#include "common.h"
#include "scanner.h"
#include "object.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

// Local typedefs
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY,
} Precedence;

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef void (*ParseFn)(bool);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;


// Global variables
static Parser parser;
static Compiler* current = NULL;
static Chunk* compilingChunk;


// Parser utilities
static void initCompiler(Compiler* compiler);
static void endCompiler(void);
static void parsePrecedence(Precedence precedence);
static ParseRule* getRule(TokenType);
static void advance(void);
static void consume(TokenType type, const char* message);
static bool match(TokenType);
static bool check(TokenType);
static uint8_t parseVariable(const char* errorMessage);
static void beginScope(void);
static void endScope(void);
static bool identifiersEqual(Token* a, Token* b);

// Error handling
static void errorAtCurrent(const char* message);
static void error(const char* message);
static void errorAt(Token* token, const char* message);
static void synchronize(void);

// Code generation
static void emitByte(uint8_t byte);
static Chunk* currentChunk(void);
static void emitReturn(void);
static void emitBytes(uint8_t, uint8_t);
static void emitConstant(Value value);
static uint8_t makeConstant(Value value);
static uint8_t identifierConstant(Token* name);
static void defineVariable(uint8_t global);
static int emitJump(uint8_t instruction);
static void patchJump(int offset);
static void emitLoop(int loopStart);

// Name resolution
static void declareVariable(void);
static void addLocal(Token name);
static int resolveLocal(Compiler* compiler, Token* name);
static void markInitialized(void);

// Expressions
static void expression(void);
static void number(bool);
static void string(bool);
static void literal(bool);
static void grouping(bool);
static void unary(bool);
static void binary(bool);
static void variable(bool canAssign);
static void namedVariable(Token name, bool canAssign);
static void and_(bool canAssign);
static void or_(bool canAssign);

// Statements
static void declaration(void);
static void block(void);
static void varDeclaration(void);
static void statement(void);
static void printStatement(void);
static void expressionStatement(void);
static void ifStatement(void);
static void whileStatement(void);
static void forStatement(void);

// Parse table
static ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]      = {grouping, NULL,     PREC_NONE},   
  [TOKEN_RIGHT_PAREN]     = {NULL,     NULL,     PREC_NONE},    
  [TOKEN_LEFT_BRACE]      = {NULL,     NULL,     PREC_NONE},   
  [TOKEN_RIGHT_BRACE]     = {NULL,     NULL,     PREC_NONE},    
  [TOKEN_COMMA]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_DOT]             = {NULL,     NULL,     PREC_NONE},
  [TOKEN_MINUS]           = {unary,    binary,   PREC_TERM},
  [TOKEN_PLUS]            = {NULL,     binary,   PREC_TERM},
  [TOKEN_SEMICOLON]       = {NULL,     NULL,     PREC_NONE},  
  [TOKEN_SLASH]           = {NULL,     binary,   PREC_FACTOR},
  [TOKEN_STAR]            = {NULL,     binary,   PREC_FACTOR},
  [TOKEN_BANG]            = {unary,    NULL,     PREC_NONE},
  [TOKEN_BANG_EQUAL]      = {NULL,     binary,   PREC_EQUALITY},  
  [TOKEN_EQUAL]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_EQUAL_EQUAL]     = {NULL,     binary,   PREC_EQUALITY},  
  [TOKEN_GREATER]         = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL]   = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS]            = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]      = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_IDENTIFIER]      = {variable, NULL,     PREC_NONE},   
  [TOKEN_STRING]          = {string,   NULL,     PREC_NONE},
  [TOKEN_NUMBER]          = {number,   NULL,     PREC_NONE},
  [TOKEN_AND]             = {NULL,     and_,     PREC_AND},
  [TOKEN_CLASS]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_ELSE]            = {NULL,     NULL,     PREC_NONE},
  [TOKEN_FALSE]           = {literal,  NULL,     PREC_NONE},
  [TOKEN_FOR]             = {NULL,     NULL,     PREC_NONE},
  [TOKEN_FUN]             = {NULL,     NULL,     PREC_NONE},
  [TOKEN_IF]              = {NULL,     NULL,     PREC_NONE},
  [TOKEN_NIL]             = {literal,  NULL,     PREC_NONE},
  [TOKEN_OR]              = {NULL,     or_,      PREC_OR},
  [TOKEN_PRINT]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_RETURN]          = {NULL,     NULL,     PREC_NONE},
  [TOKEN_SUPER]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_THIS]            = {NULL,     NULL,     PREC_NONE},
  [TOKEN_TRUE]            = {literal,  NULL,     PREC_NONE},
  [TOKEN_VAR]             = {NULL,     NULL,     PREC_NONE},
  [TOKEN_WHILE]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_ERROR]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_EOF]             = {NULL,     NULL,     PREC_NONE},
};

///////////////////////////////////////

// Implementation

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF))
    declaration();

  endCompiler();

  return !parser.hadError;
}

static void advance(void) {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;

  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF)
    fprintf(stderr, " at end");
  else if (token->type == TOKEN_ERROR)
    ; // Nothing
  else
    fprintf(stderr, " at '%.*s'", token->length, token->start);

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static Chunk* currentChunk(void) {
  return compilingChunk;
}

static void initCompiler(Compiler* compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  current = compiler;
}

static void endCompiler(void) {
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError)
    disassembleChunk(currentChunk(), "code");
#endif
}

static void emitReturn(void) {
  emitByte(OP_RETURN);
}

static void emitBytes(uint8_t b1, uint8_t b2) {
  emitByte(b1);
  emitByte(b2);
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk");
    return 0;
  }

  return (uint8_t) constant;
}

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (!prefixRule) {
    error("Expect expression");
    return;
  }
  
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target");
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

static void expression(void) {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void number(bool x) {
  (void)x;
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void string(bool x) {
  (void)x;
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
          parser.previous.length - 2)));
}

static void literal(bool x) {
  (void)x;
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default: assert(false); // unreachable
  }
}

static void grouping(bool x) {
  (void)x;
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

static void unary(bool x) {
  (void)x;
  TokenType operatorType = parser.previous.type;

  // Compile the operand
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction
  switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: assert(false); // unreachable
  }
}

static void binary(bool x) {
  (void)x;
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence) (rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT);  break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL);           break;
    case TOKEN_GREATER:       emitByte(OP_GREATER);         break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT);   break;
    case TOKEN_LESS:          emitByte(OP_LESS);            break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT);break;
    case TOKEN_PLUS:          emitByte(OP_ADD);             break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT);        break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY);        break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE);          break;
    default: assert(false); // Unreachable
  }
}

static void declaration(void) { 
  if (match(TOKEN_VAR))
    varDeclaration();
  else
    statement(); 

  if (parser.panicMode) 
    synchronize();
}

static void statement(void) {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static bool check(TokenType type) {
  return parser.current.type == type;
}

static void printStatement(void) {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value");
  emitByte(OP_PRINT);
}

static void expressionStatement(void) {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression");
  emitByte(OP_POP);
}

static void synchronize(void) {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN: 
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default: ;
    }
    advance();
  }
}

static void varDeclaration(void) {
  uint8_t global = parseVariable("Expect variable name");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, 
      "Expect ';' after variable declaration");
  defineVariable(global);
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}

static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(
        copyString(name->start, name->length)));
}

static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);

  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }


  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t) arg);
  } else {
    emitBytes(getOp, (uint8_t) arg);
  }
}

static void block(void) {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    declaration();

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block");
}

static void beginScope(void) {
  current->scopeDepth++;
}

static void endScope(void) {
  current->scopeDepth--;

  while (current->localCount > 0 &&
      current->locals[current->localCount - 1].depth
      > current->scopeDepth) {
    emitByte(OP_POP);
    current->localCount--;
  }
}

static void declareVariable(void) {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;

  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth)
      break;

    if (identifiersEqual(name, &local->name))
      error("Already a variable with this name in this scope");
  }

  addLocal(*name);
}

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
}

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer");
      }
      return i;
    }
  }

  return -1;
}

static void markInitialized(void) {
  current->locals[current->localCount - 1].depth =
    current->scopeDepth;
}

static void ifStatement(void) {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TOKEN_ELSE)) statement();
  patchJump(elseJump);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->code.size - 2;
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself
  int jump = currentChunk()->code.size - 2;
  if (jump > UINT16_MAX)
    error("Too much code to jump over");

  currentChunk()->code.data[offset] = (jump >> 8) & 0xff;
  currentChunk()->code.data[offset+1] = jump & 0xff;
}


static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecedence(PREC_AND);
  patchJump(endJump);
}

static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void whileStatement(void) {
  int loopStart = currentChunk()->code.size;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->code.size - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static void forStatement(void) {
  beginScope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'");
  if (match(TOKEN_SEMICOLON)) {
    // No initializer
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->code.size;
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
  }
  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->code.size;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP);
  }

  endScope();
}
