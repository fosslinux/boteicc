#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "M2libc/bootstrappable.h"

#define FALSE 0
#define TRUE 1

int main(int argc, char **argv) {
	if (argc != 2) {
		fputs("invalid number of arguments\n", stderr);
		return 1;
	}

	char *a = calloc(strlen(argv[1]), sizeof(char));
	strcpy(a, argv[1]);

	fputs("int main() { return ", stdout);
	char *b = a;
	while (*b) {
		while (*b >= 48 && *b <= 57) {
			fputc(*b, stdout);
			b += 1;
		}
    	if (*b == '+') {
    		fputc('+', stdout);
			b += 1;
			continue;
    	}
		if (*b == '-') {
			fputc('-', stdout);
			b += 1;
			continue;
		}
		if (!*b) {
			b += 1;
			break;
		}

		fputs("unexpected character\n", stderr);
	}

	fputs("; }\n", stdout);
	return 0;
}
