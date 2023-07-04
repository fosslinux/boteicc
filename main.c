#include "chibicc.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		error("invalid number of arguments\n");
	}

	Token *tok = tokenize(argv[1]);
	Node *node = parse(tok);
	codegen(node);

	// Traverse the AST.
	return 0;
}
