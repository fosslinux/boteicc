#include "chibicc.h"

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

// Read a punctuator toekn from p and return its length.
int read_punct(char *p) {
	if (startswith(p, "==") || startswith(p, "!=") ||
		startswith(p, "<=") || startswith(p, ">=")) {
		return 2;
	}

	if (isapunct(*p)) {
		return 1;
	} else {
		return 0;
	}
}

// Tokenize `p` and return new tokens.
Token *tokenize(char *p) {
	current_input = p;
	Token *head = calloc(1, sizeof(Token));
	Token *cur = head;

	char *end;
	int punct_len;
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
		punct_len = read_punct(p);
		if (punct_len) {
			cur->next = new_token(TK_PUNCT, p, p + punct_len);
			cur = cur->next;
			p += cur->len;
			continue;
		}

		error_at(p, "invalid token");
	}

	cur->next = new_token(TK_EOF, p, p);
	cur = cur->next;
	return head->next;
}
