#include "chibicc.h"

char *opt_o;

char *input_path;

void usage(int status) {
	fputs("chibicc [ -o <path> ] <file>\n", stderr);
	exit(status);
}

void parse_args(int argc, char **argv) {
	int i;
	char *msg;
	for (i = 1; i < argc; i += 1) {
		if (!strcmp(argv[i], "--help")) {
			usage(0);
		}

		if (!strcmp(argv[i], "-o")) {
			i += 1;
			if (!argv[i]) {
				usage(1);
			}
			opt_o = argv[i];
			continue;
		}

		if (!strncmp(argv[i], "-o", 2)) {
			opt_o = argv[i] + 2;
			continue;
		}

		if (argv[i][0] == '-' && argv[i][1] != '\0') {
			msg = calloc(MAX_STRING, sizeof(char));
			strcpy(msg, "unknown argument: ");
			strcat(msg, argv[i]);
			error(msg);
		}

		input_path = argv[i];
	}

	if (!input_path) {
		error("no input file given");
	}
}

FILE *open_file(char *path) {
	if (!path) {
		return stdout;
	}
	if (strcmp(path, "-") == 0) {
		return stdout;
	}

	FILE *out = fopen(path, "w");
	if (!out) {
		char *msg = calloc(MAX_STRING, sizeof(char));
		strcpy(msg, "cannot open output file: ");
		strcat(msg, path);
		error(msg);
	}

	return out;
}

int main(int argc, char **argv) {
	// Initialisation
	initialize_types();

	parse_args(argc, argv);

	// Tokenize, parse.
	Token *tok = tokenize_file(input_path);
	Obj *prog = parse(tok);

	// Traverse the AST, emitting assembly.
	FILE *out = open_file(opt_o);
	codegen(prog, out);

	return 0;
}
