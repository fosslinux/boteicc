#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "M2libc/bootstrappable.h"

#define FALSE 0
#define TRUE 1
#define MAX_STRING 4096

#define TK_PUNCT 0
#define TK_NUM 1
#define TK_EOF 2

//
// HELPER FUNCTIONS
//

// isadigit
int isadigit(char c) {
	return (c >= 48 && c <= 57);
}

// isagraph
int isagraph(char c) {
	return (c >= 32 && c <= 126);
}

// isaalpha
int isaalpha(char c) {
	return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

// isapunct
int isapunct(char c) {
	return (isagraph(c) && !(isaalpha(c) || isadigit(c)));
}

// Get slice
char *string_slice(char *original, char *end) {
	int diff = end - original;
	char *slice = calloc(strlen(original), sizeof(char));
	strcpy(slice, original);
	slice[diff] = '\0';
	return slice;
}

// Get integer end
char *integer_end(char *s) {
	while (isadigit(*s)) {
		s += 1;
	}
	return s;
}

char *uint2str(int i) {
	return int2str(i, 10, FALSE);
}

//
// TOKENIZER
//

// Token type
struct Token {
	int kind;    // Token kind
    struct Token *next; // Next token
	int val;     // If kind is TK_NUM, its value
	char *loc;   // Token location
	int len;     // Token length
};
typedef struct Token Token;

// Input string
char *current_input;

// Reports an error and exit.
void error(char *fmt) {
	fputs(fmt, stderr);
	fputc('\n', stderr);
	exit(1);
}

// Reports an error location and exists.
void error_at(char *loc, char *fmt) {
	int pos = loc - current_input;
	fputs(current_input, stderr);
	fputc('\n', stderr);
	int i;
	for (i = 0; i < pos; i += 1) {
		fputc(' ', stderr);
	}
	fputs("^ ", stderr);
	error(fmt);
}

void error_tok(Token *tok, char *fmt) {
	error_at(tok->loc, fmt);
}

// Consumes the current tyoken if it matches `s`??
int equal(Token *tok, char *op) {
	return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `s`.
Token *skip(Token *tok, char *s) {
	if (!equal(tok, s)) {
		error_tok(tok, strcat("expected ", s));
	}
	return tok->next;
}

// Ensure current token is TK_NUM.
int get_number(Token *tok) {
	if (tok->kind != TK_NUM) {
		error_tok(tok, "expected a number");
	}
	return tok->val;
}

// Create a new token.
Token *new_token(int kind, char *start, char *end) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->loc = start;
	tok->len = end - start;
	return tok;
}

// Tokenize `p` and return new tokens.
Token *tokenize(void) {
	char *p = current_input;
	Token *head = calloc(1, sizeof(Token));
	Token *cur = head;

	char *end;
	while (*p) {
		// Skip whitespace characters.
		if (isspace(*p)) {
			p += 1;
			continue;
		}

		// Numeric literal
		if (isadigit(*p)) {
			cur->next = new_token(TK_NUM, p, p);
			cur = cur->next;
			end = integer_end(p);
			cur->val = strtoint(string_slice(p, end));
			cur->len = end - p;
			p = end;
			continue;
		}

		// Punctuators
		if (isapunct(*p)) {
			cur->next = new_token(TK_PUNCT, p, p + 1);
			cur = cur->next;
			p += 1;
			continue;
		}

		error_at(p, "invalid token");
	}

	cur->next = new_token(TK_EOF, p, p);
	cur = cur->next;
	return head->next;
}

//
// PARSER
//

#define ND_ADD 0 // +
#define ND_SUB 1 // -
#define ND_MUL 2 // *
#define ND_DIV 3 // /
#define ND_NUM 4 // Integer

// AST node type
struct Node {
	int kind;         // Node kind
	struct Node *lhs; // Left-hand side
	struct Node *rhs; // Right-hand side
	int val;          // Used if kind == ND_NUM
};
typedef struct Node Node;

Node *new_node(int kind) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

Node *new_binary(int kind, Node *lhs, Node *rhs) {
	Node *node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_num(int val) {
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

Node *expr(Token **rest, Token *tok);
Node *mul(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);

// expr = mul ("+" mul | "-" mul)*
Node *expr(Token **rest, Token *tok) {
	Node *node = mul(&tok, tok);

	while (1) {
		if (equal(tok, "+")) {
			node = new_binary(ND_ADD, node, mul(&tok, tok->next));
			continue;
		}

		if (equal(tok, "-")) {
			node = new_binary(ND_SUB, node, mul(&tok, tok->next));
			continue;
		}

		rest[0] = tok;
		return node;
	}
}

// mul = primary ("*" primary | "/" primary)*
Node *mul(Token **rest, Token *tok) {
	Node *node = primary(&tok, tok);

	while (1) {
		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, primary(&tok, tok->next));
			continue;
		}

		if (equal(tok, "/")) {
			node = new_binary(ND_DIV, node, primary(&tok, tok->next));
			continue;
		}

		rest[0] = tok;
		return node;
	}
}

// primary = "(" expr ")" | num
Node *primary(Token **rest, Token *tok) {
	if (equal(tok, "(")) {
		Node *node = expr(&tok, tok->next);
		rest[0] = skip(tok, ")");
		return node;
	}

	if (tok->kind == TK_NUM) {
		Node *node = new_num(tok->val);
		rest[0] = tok->next;
		return node;
	}

	error_tok(tok, "expected an expression");
}

//
// Code generator
//

// in a way, simulates x86_64

int function = 1;

int function_def(char *str) {
	fputs("int f", stdout);
	fputs(int2str(function, 10, FALSE), stdout);
	fputs("() { ", stdout);
	fputs(str, stdout);
	fputs("}\n", stdout);
	function += 1;
	return function - 1;
}

char *operation(int lhs, int rhs, char *symbol) {
	char *full = calloc(MAX_STRING, sizeof(char));
	strcpy(full, "return f");
	strcat(full, uint2str(lhs));
	strcat(full, "()");
	strcat(full, symbol);
	strcat(full, "f");
	strcat(full, uint2str(rhs));
	strcat(full, "(); ");
	return full;
}

int gen_expr(Node *node) {
	if (node->kind == ND_NUM) {
		char *full = calloc(MAX_STRING, sizeof(char));
		strcpy(full, "return ");
		strcat(full, uint2str(node->val));
		strcat(full, "; ");
		return function_def(full);
	}

	int rhs = gen_expr(node->rhs);
	int lhs = gen_expr(node->lhs);
	if (node->kind == ND_ADD) {
		return function_def(operation(lhs, rhs, "+"));
	} else if (node->kind == ND_SUB) {
		return function_def(operation(lhs, rhs, "-"));
	} else if (node->kind == ND_MUL) {
		return function_def(operation(lhs, rhs, "*"));
	} else if (node->kind == ND_DIV) {
		return function_def(operation(lhs, rhs, "/"));
	}

	error("invalid expression");
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fputs("invalid number of arguments\n", stderr);
		return 1;
	}

	current_input = argv[1];
	Token *tok = tokenize();
	Node *node = expr(&tok, tok);

	if (tok->kind != TK_EOF) {
		error_tok(tok, "extra token");
	}

	// Traverse the AST.
	int main = gen_expr(node);
	fputs("int main() { return f", stdout);
	fputs(uint2str(main), stdout);
	fputs("(); }\n", stdout);

	return 0;
}
