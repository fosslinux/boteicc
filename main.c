#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	if (argc != 2) {
		fputs("invalid number of arguments\n", stderr);
		return 1;
	}

	fputs("int main() { return ", stdout);
	fputs(argv[1], stdout);
	fputs("; }\n", stdout);
	return 0;
}
