#include "chibicc.h"

// isadigit
int isadigit(char c) {
	return '0' <= c && c <= '9';
}

// isxdigit
int isaxdigit(char c) {
	return isadigit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

// isagraph
int isagraph(char c) {
	return c >= 32 && c <= 126;
}

// isaalpha
int isaalpha(char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

// isapunct
int isapunct(char c) {
	return isagraph(c) && !(isaalpha(c) || isadigit(c));
}

// isaxdigit
int isaxdigt(char c) {
	return ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

// ctolower
char ctolower(char c) {
	if ('A' <= c && c <= 'Z') {
		return c + 32;
	} else {
		return c;
	}
}

char *stolower(char *s) {
	char *out = calloc(strlen(s) + 1, sizeof(char));
	char *result = out;
	strcpy(out, s);
	while (s[0] != '\0') {
		out[0] = ctolower(*s);
		out += 1;
		s += 1;
	}
	out[0] = '\0';
	return result;
}

// startswith
int startswith(char *p, char *q) {
	return strncmp(p, q, strlen(q)) == 0;
}

// Get slice
char *string_slice(char *original, char *end) {
	int diff = end - original;
	char *slice = calloc(strlen(original) + 1, sizeof(char));
	strcpy(slice, original);
	slice[diff] = '\0';
	return slice;
}

char *uint2str(int i) {
	return int2str(i, 10, FALSE);
}
