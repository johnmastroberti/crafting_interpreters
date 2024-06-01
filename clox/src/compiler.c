#include <stdio.h>
#include <assert.h>
#include "compiler.h"
#include "common.h"
#include "scanner.h"
#include "object.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

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

typedef void (*ParseFn)(void);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static Parser parser;
static Chunk* compilingChunk;


static void advance(void);
static void errorAtCurrent(const char* message);
static void error(const char* message);
static void errorAt(Token* token, const char* message);
static void consume(TokenType type, const char* message);
static void emitByte(uint8_t byte);
static Chunk* currentChunk(void);
static void endCompiler(void);
static void emitReturn(void);
static void emitBytes(uint8_t, uint8_t);
static void expression(void);
static void number(void);
static void string(void);
static void literal(void);
static void emitConstant(Value value);
static uint8_t makeConstant(Value value);
static void grouping(void);
static void unary(void);
static void parsePrecedence(Precedence precedence);
static void binary(void);
static ParseRule* getRule(TokenType);

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
  [TOKEN_IDENTIFIER]      = {NULL,     NULL,     PREC_NONE},   
  [TOKEN_STRING]          = {string,   NULL,     PREC_NONE},
  [TOKEN_NUMBER]          = {number,   NULL,     PREC_NONE},
  [TOKEN_AND]             = {NULL,     NULL,     PREC_NONE},
  [TOKEN_CLASS]           = {NULL,     NULL,     PREC_NONE},
  [TOKEN_ELSE]            = {NULL,     NULL,     PREC_NONE},
  [TOKEN_FALSE]           = {literal,  NULL,     PREC_NONE},
  [TOKEN_FOR]             = {NULL,     NULL,     PREC_NONE},
  [TOKEN_FUN]             = {NULL,     NULL,     PREC_NONE},
  [TOKEN_IF]              = {NULL,     NULL,     PREC_NONE},
  [TOKEN_NIL]             = {literal,  NULL,     PREC_NONE},
  [TOKEN_OR]              = {NULL,     NULL,     PREC_NONE},
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

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();
  expression();
  consume(TOKEN_EOF, "Expect end of expression");
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
  prefixRule();

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

static void expression(void) {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void number(void) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void string(void) {
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
          parser.previous.length - 2)));
}

static void literal(void) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default: assert(false); // unreachable
  }
}

static void grouping(void) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

static void unary(void) {
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

static void binary(void) {
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
