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

// Token
#define TK_IDENT   0 // Identifiers
#define TK_PUNCT   1 // Punctuators
#define TK_KEYWORD 2 // Keywords
#define TK_NUM     3 // Numeric literals
#define TK_EOF     4 // End-of-file markers

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
int get_number(Token *tok);
Token *skip(Token *tok, char *op);
int consume(Token **rest, Token *tok, char *str);
Token *tokenize(char *input);

//
// type.c
//

#define TY_INT   0
#define TY_PTR   1
#define TY_FUNC  2
#define TY_ARRAY 3

// XXX Ensure copy_type is updated when a field is added.
struct Type {
	int kind;

	int size; // sizeof() value

	// Pointer-to or array-of type. Same member used to represent the
	// pointer/array duality in C.
	//
	// In many contexts where a pointer is expected, we examine this member
	// instead of "kind" to determine whether a type is a pointer or not. That
	// means in many contexts "array of T" is naturally handled as if it was a
	// "pointer to T", as required by the C spec.
	struct Type *base;

	// Declaration
	Token *name;

	// Array
	int array_len;

	// Function
	struct Type *return_ty;
	struct Type *params;
	struct Type *next;
};
typedef struct Type Type;

extern Type *ty_int;

int is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
void initialize_types(void);

//
// parse.c
//

// Local variable
struct Obj {
	struct Obj *next;
	char *name; // Variable name
	Type *ty;   // Type
	int offset; // Offset from EBP
};
typedef struct Obj Obj;

// AST node
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
#define ND_RETURN    13 // "return"
#define ND_BLOCK     14 // { ... }
#define ND_IF        15 // "if"
#define ND_FOR       16 // "for" or "while"
#define ND_ADDR      17 // unary &
#define ND_DEREF     18 // unary *
#define ND_FUNCALL   19 // Function call

// AST node type
struct Node {
	int kind;          // Node kind
	struct Node *next; // Next node
	Type *ty;          // Type, e.g. int or pointer to int
	Token *tok;        // Representative token

	struct Node *lhs;  // Left-hand side
	struct Node *rhs;  // Right-hand side

	// "if" or "for" statement
	struct Node *cond;
	struct Node *then;
	struct Node *els;
	struct Node *init;
	struct Node *inc;

	// Block
	struct Node *body;

	// Function call
	char *funcname;
	struct Node *args;

	Obj *var;          // Used if kind == ND_VAR
	int val;           // Used if kind == ND_NUM
};
typedef struct Node Node;

// Function
struct Function {
	struct Function *next;
	char *name;
	Obj *params;

	Node *body;
	Obj *locals;
	int stack_size;
};
typedef struct Function Function;

Function *parse(Token *tok);

//
// type.c
//

void add_type(Node *node);

//
// codegen.c
//

void codegen(Function *prog);

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
void str_postfix(char *str, char *second);
void num_postfix(char *str, int c);
