#include "chibicc.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		error("invalid number of arguments\n");
	}

	// Initialisation
	initialize_types();

	Token *tok = tokenize(argv[1]);
	Function *prog = parse(tok);

	// Traverse the AST, emitting assembly.
	codegen(prog);

	return 0;
}
