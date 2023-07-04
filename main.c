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

// isadigit
int isadigit(char c) {
	return (c >= 48 && c <= 57);
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

// Token type
struct Token {
	int kind;    // Token kind
    struct Token *next; // Next token
	int val;     // If kind is TK_NUM, its value
	char *loc;   // Token location
	int len;     // Token length
};
typedef struct Token Token;

// Reports an error and exit.
void error(char *fmt) {
	fputs(fmt, stderr);
	fputc('\n', stderr);
	exit(1);
}

// Consumes the current tyoken if it matches `s`??
int equal(Token *tok, char *op) {
	return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `s`.
Token *skip(Token *tok, char *s) {
	if (!equal(tok, s)) {
		error(strcat("expected ", s));
	}
	return tok->next;
}

// Ensure current token is TK_NUM.
int get_number(Token *tok) {
	if (tok->kind != TK_NUM) {
		error("expected a number");
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
Token *tokenize(char *p) {
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

		// Punctuator
		if (*p == '+' || *p == '-') {
			cur->next = new_token(TK_PUNCT, p, p + 1);
			cur = cur->next;
			p += 1;
			continue;
		}

		error("invalid token");
	}

	cur->next = new_token(TK_EOF, p, p);
	cur = cur->next;
	return head->next;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fputs("invalid number of arguments\n", stderr);
		return 1;
	}

	Token *tok = tokenize(argv[1]);
	fputs("int main() { return ", stdout);

	// First token is a number
	fputs(int2str(get_number(tok), 10, FALSE), stdout);
	tok = tok->next;

	// ... followed by either `+ <number>` or `- <number>`.
	while (tok->kind != TK_EOF) {
		if (equal(tok, "+")) {
			fputc('+', stdout);
			fputs(int2str(get_number(tok->next), 10, FALSE), stdout);
			tok = tok->next->next;
			continue;
		}

		tok = skip(tok, "-");
		fputc('-', stdout);
		fputs(int2str(get_number(tok), 10, FALSE), stdout);
		tok = tok->next;
	}

	fputs("; }\n", stdout);
	return 0;
}
