#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "M2libc/bootstrappable.h"

#define FALSE 0
#define TRUE 1
#define MAX_STRING 4096

// Struct typedefs
typedef struct sMember Member;
typedef struct sType Type;
typedef struct sNode Node;
typedef struct sToken Token;
typedef struct sObj Obj;
typedef struct sNode Node;

//
// tokenize.c
//

// Token
#define TK_IDENT   0 // Identifiers
#define TK_PUNCT   1 // Punctuators
#define TK_KEYWORD 2 // Keywords
#define TK_NUM     3 // Numeric literals
#define TK_EOF     4 // End-of-file markers
#define TK_STR     5 // String literals

// Token type
struct sToken {
	int kind;           // Token kind
	Token *next;        // Next token
	int32_t val;        // If kind is TK_NUM, its value
	char *loc;          // Token location
	int len;            // Token length
	Type *ty;           // Used if TK_STR
	char *str;          // String literal contents including terminator

	int line_no;        // Line number
};

void error(char *fmt);
void error_at(char *loc, char *fmt);
void error_tok(Token *tok, char *fmt);
int equal(Token *tok, char *op);
long get_number(Token *tok);
Token *skip(Token *tok, char *op);
int consume(Token **rest, Token *tok, char *str);
Token *tokenize_file(char *filename);

//
// type.c
//

#define TY_INT    0
#define TY_PTR    1
#define TY_FUNC   2
#define TY_ARRAY  3
#define TY_CHAR   4
#define TY_STRUCT 5
#define TY_UNION  6
#define TY_LONG   7
#define TY_SHORT  8
#define TY_VOID   9
#define TY_BOOL  10
#define TY_ENUM  11

// XXX Ensure copy_type is updated when a field is added.
struct sType {
	int kind;

	int size; // sizeof() value
	int align; // alignment

	// Pointer-to or array-of type. Same member used to represent the
	// pointer/array duality in C.
	//
	// In many contexts where a pointer is expected, we examine this member
	// instead of "kind" to determine whether a type is a pointer or not. That
	// means in many contexts "array of T" is naturally handled as if it was a
	// "pointer to T", as required by the C spec.
	Type *base;

	// Declaration
	Token *name;

	// Array
	int array_len;

	// Struct
	Member *members;

	// Function
	Type *return_ty;
	Type *params;
	Type *next;
};

extern Type *ty_void;
extern Type *ty_bool;
extern Type *ty_int;
extern Type *ty_short;
extern Type *ty_char;
extern Type *ty_long;

// Struct member
struct sMember {
	Member *next;
	Type *ty;
	Token *tok;  // for error message
	Token *name;
	int offset;
};

int is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
Type *enum_type(void);
Type *struct_type(void);
void initialize_types(void);

//
// parse.c
//

// Variable or function
struct sObj {
	Obj *next;
	char *name;   // Variable name
	Type *ty;     // Type
	int is_local; // local or global/function

	// Local variable
	int offset; // Offset from EBP

	// Global variable or function
	int is_function;
	int is_definition;
	int is_static;

	// Global variable
	char *init_data;

	// Function
	Obj *params;
	Node *body;
	Obj *locals;
	int stack_size;
};

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
#define ND_STMT_EXPR 20 // Statement expression [GNU]
#define ND_COMMA     21 // ,
#define ND_MEMBER    22 // .
#define ND_CAST      23 // Type cast
#define ND_NOT       24 // !
#define ND_BITNOT    25 // ~
#define ND_MOD       26 // %
#define ND_BITAND    27 // &
#define ND_BITOR     28 // |
#define ND_BITXOR    29 // ^
#define ND_LOGAND    30 // &&
#define ND_LOGOR     31 // ||
#define ND_GOTO      32 // "goto"
#define ND_LABEL     33 // Labeled statement
#define ND_SWITCH    34 // "switch"
#define ND_CASE      35 // "case"
#define ND_SHL       36 // <<
#define ND_SHR       37 // >>
#define ND_COND      38 // ?:
#define ND_NULL_EXPR 39 // Do nothing

// AST node type
struct sNode {
	int kind;          // Node kind
	Node *next;        // Next node
	Type *ty;          // Type, e.g. int or pointer to int
	Token *tok;        // Representative token

	Node *lhs;         // Left-hand side
	Node *rhs;         // Right-hand side

	// "if" or "for" statement
	Node *cond;
	Node *then;
	Node *els;
	Node *init;
	Node *inc;

	// "break" and "continue" labels
	char *brk_label;
	char *cont_label;

	// Block or statement expression
	Node *body;

	// Struct member access
	Member *member;

	// Function call
	char *funcname;
	Type *func_ty;
	Node *args;

	// Goto or labeled statement
	char *label;
	char *unique_label;
	Node *goto_next;

	// Switch-cases
	Node *case_next;
	Node *default_case;

	// Variable
	Obj *var;

	// Numeric literal
	int32_t val;
};

Node *new_cast(Node *expr, Type *ty);
Obj *parse(Token *tok);

//
// type.c
//

void add_type(Node *node);

//
// codegen.c
//

void codegen(Obj *prog, FILE *out);
int align_to(int n, int align);

//
// util.c
//

int isadigit(char c);
int isaxdigit(char c);
int isagraph(char c);
int isaalpha(char c);
int isapunct(char c);
int startswith(char *p, char *q);
char *string_slice(char *original, char *end);
char ctolower(char c);
char *stolower(char *s);
char *uint2str(int i);
void str_postfix(char *str, char *second);
void num_postfix(char *str, int c);
