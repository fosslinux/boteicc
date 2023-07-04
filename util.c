#include "chibicc.h"

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

// startswith
int startswith(char *p, char *q) {
	return strncmp(p, q, strlen(q)) == 0;
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
