#include "chibicc.h"

int opt_cc1;
int opt_hash_hash_hash;
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
		if (!strcmp(argv[i], "-###")) {
			opt_hash_hash_hash = TRUE;
			continue;
		}

		if (!strcmp(argv[i], "-cc1")) {
			opt_cc1 = TRUE;
			continue;
		}

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

void run_subprocess(char **argv) {
	// If -### is given, dump the subprocess command line.
	if (opt_hash_hash_hash) {
		fputs(argv[0], stderr);
		int i;
		for (i = 1; argv[i]; i += 1) {
			fputc(' ', stderr);
			fputs(argv[i], stderr);
		}
		fputc('\n', stderr);
	}

	int pid = fork();
	if (pid == 0) {
		// Child proc
		execve(argv[0], argv, NULL);
		fputs("exec failed: ", stderr);
		fputs(argv[0], stderr);
		fputc('\n', stderr);
		exit(1);
	}

	// Wait for child process.
	int status;
	while (waitpid(pid, &status, 0) > 0) {}
	if (status != 0) {
		exit(1);
	}
}

void run_cc1(int argc, char **argv) {
	char **args = calloc(argc + 10, sizeof(char*));
	memcpy(args, argv, argc * sizeof(char*));
	args[argc] = "-cc1";
	argc += 1;
	run_subprocess(args);
}

void cc1(void) {
	// Initialisation
	initialize_types();

	// Tokenize, parse.
	Token *tok = tokenize_file(input_path);
	Obj *prog = parse(tok);

	// Traverse the AST, emitting assembly.
	FILE *out = open_file(opt_o);
	codegen(prog, out);
}

int main(int argc, char **argv) {
	parse_args(argc, argv);

	if (opt_cc1) {
		cc1();
		return 0;
	}

	run_cc1(argc, argv);
	return 0;
}
