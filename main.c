#include "chibicc.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		error("invalid number of arguments\n");
	}

	// Initialisation
	initialize_types();

	// Tokenize, parse.
	Token *tok = tokenize(argv[1]);
	Obj *prog = parse(tok);

	// Traverse the AST, emitting assembly.
	codegen(prog);

	return 0;
}
