#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "M2libc/bootstrappable.h"

#define FALSE 0
#define TRUE 1
#define MAX_STRING 4096

//
// tokenize.c
//

#define TK_IDENT 0 // Identifiers
#define TK_PUNCT 1 // Punctuators
#define TK_NUM   2 // Numeric literals
#define TK_EOF   3 // End-of-file markers

// Token type
struct Token {
	int kind;    // Token kind
    struct Token *next; // Next token
	int val;     // If kind is TK_NUM, its value
	char *loc;   // Token location
	int len;     // Token length
};
typedef struct Token Token;

void error(char *fmt);
void error_at(char *loc, char *fmt);
void error_tok(Token *tok, char *fmt);
int equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
Token *tokenize(char *input);

//
// parse.c
//

#define ND_ADD        0 // +
#define ND_SUB        1 // -
#define ND_MUL        2 // *
#define ND_DIV        3 // /
#define ND_NUM        4 // Integer
#define ND_NEG        5 // unary -
#define ND_EQ         6 // ==
#define ND_NE         7 // !=
#define ND_LT         8 // <
#define ND_LE         9 // <=
#define ND_EXPR_STMT 10 // Expression statement
#define ND_ASSIGN    11 // =
#define ND_VAR       12 // Variable

// AST node type
struct Node {
	int kind;          // Node kind
	struct Node *next; // Next node
	struct Node *lhs;  // Left-hand side
	struct Node *rhs;  // Right-hand side
	char name;         // Used if kind == ND_VAR
	int val;           // Used if kind == ND_NUM
};
typedef struct Node Node;

Node *parse(Token *tok);

//
// codegen.c
//

void codegen(Node *node);

//
// util.c
//

int isadigit(char c);
int isagraph(char c);
int isaalpha(char c);
int isapunct(char c);
int startswith(char *p, char *q);
char *string_slice(char *original, char *end);
char *integer_end(char *s);
char *uint2str(int i);
