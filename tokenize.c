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
		char *err = calloc(MAX_STRING, sizeof(char));
		strcpy(err, "expected ");
		strcat(err, s);
		error_tok(tok, err);
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

int consume(Token **rest, Token *tok, char *str) {
	if (equal(tok, str)) {
		*rest = tok->next;
		return TRUE;
	}

	*rest = tok;
	return FALSE;
}

// Create a new token.
Token *new_token(int kind, char *start, char *end) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->loc = start;
	tok->len = end - start;
	return tok;
}

// Returns true if c is valid as the first character of an identifier.
int is_ident1(char c) {
	return isaalpha(c) || c == '_';
}

// Returns true if c is valid as a non-first character of an identifier;
int is_ident2(char c) {
	return is_ident1(c) || isadigit(c);
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

int is_keyword(Token *tok) {
	if (equal(tok, "return") ||
			equal(tok, "if") ||
			equal(tok, "else") ||
			equal(tok, "for") ||
			equal(tok, "while") ||
			equal(tok, "int") ||
			equal(tok, "sizeof") ||
			equal(tok, "char")) {
		return TRUE;
	}
	return FALSE;
}

int read_escaped_char(char *p) {
	// Escape sequences are generally defined using themselves here.
	if (*p == 'a') {
		return '\a';
	} else if (*p == 'b') {
		return '\b';
	} else if (*p == 't') {
		return '\t';
	} else if (*p == 'n') {
		return '\n';
	} else if (*p == 'v') {
		return '\v';
	} else if (*p == 'f') {
		return '\f';
	} else if (*p == 'r') {
		return '\r';
	} else if (*p == 'e') {
		return 27; // ASCII escape character [GNU]
	} else {
		return *p;
	}
}

// Find a closing double-quote.
char *string_literal_end(char *p) {
	char *start = p;
	for (p; *p != '"'; p += 1) {
		if (*p == '\n' || *p == '\0') {
			error_at(start, "unclosed string literal");
		}
		if (*p == '\\') {
			p += 1;
		}
	}
	return p;
}

Token *read_string_literal(char *start) {
	char *end = string_literal_end(start + 1);
	char *buf = calloc(1, end - start);
	int len = 0;

	char *p;
	for (p = start + 1; p < end; p) {
		if (*p == '\\') {
			buf[len] = read_escaped_char(p + 1);
			len += 1;
			p += 2;
		} else {
			buf[len] = *p;
			len += 1;
			p += 1;
		}
	}

	Token *tok = new_token(TK_STR, start, end + 1);
	tok->ty = array_of(ty_char, len + 1);
	tok->str = buf;
	return tok;
}

void convert_keywords(Token *tok) {
	Token *t;
	for (t = tok; t->kind != TK_EOF; t = t->next) {
		if (is_keyword(t)) {
			t->kind = TK_KEYWORD;
		}
	}
}

// Tokenize a given string and return new tokens.
Token *tokenize(char *p) {
	current_input = p;
	Token *head = calloc(1, sizeof(Token));
	Token *cur = head;

	char *start;
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

		// String literal
		if (*p == '"') {
			cur->next = read_string_literal(p);
			cur = cur->next;
			p += cur->len;
			continue;
		}

		// Identifier or keyword
		if (is_ident1(*p)) {
			start = p;
			do {
				p += 1;
			} while (is_ident2(*p));
			cur->next = new_token(TK_IDENT, start, p);
			cur = cur->next;
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
	convert_keywords(head->next);
	return head->next;
}
