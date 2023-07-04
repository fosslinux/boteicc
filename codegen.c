#include "chibicc.h"

int function = 1;

int function_def(char *str) {
	fputs("int f", stdout);
	fputs(uint2str(function), stdout);
	fputs("() { ", stdout);
	fputs(str, stdout);
	fputs("}\n", stdout);
	function += 1;
	return function - 1;
}

char *operation(int lhs, int rhs, char *symbol) {
	char *full = calloc(MAX_STRING, sizeof(char));
	strcpy(full, "return f");
	strcat(full, uint2str(lhs));
	strcat(full, "()");
	strcat(full, symbol);
	strcat(full, "f");
	strcat(full, uint2str(rhs));
	strcat(full, "(); ");
	return full;
}

int gen_expr(Node *node) {
	// Sideless
	if (node->kind == ND_NUM) {
		char *full = calloc(MAX_STRING, sizeof(char));
		strcpy(full, "return ");
		strcat(full, uint2str(node->val));
		strcat(full, "; ");
		return function_def(full);
	}

	// Unary
	int lhs = gen_expr(node->lhs);

	if (node->kind == ND_NEG) {
		char *full = calloc(MAX_STRING, sizeof(char));
		strcpy(full, "return -f");
		strcat(full, uint2str(lhs));
		strcat(full, "(); ");
		return function_def(full);
	}

	// Binary
	int rhs = gen_expr(node->rhs);

	if (node->kind == ND_ADD) {
		return function_def(operation(lhs, rhs, "+"));
	} else if (node->kind == ND_SUB) {
		return function_def(operation(lhs, rhs, "-"));
	} else if (node->kind == ND_MUL) {
		return function_def(operation(lhs, rhs, "*"));
	} else if (node->kind == ND_DIV) {
		return function_def(operation(lhs, rhs, "/"));
	} else if (node->kind == ND_EQ) {
		return function_def(operation(lhs, rhs, "=="));
	} else if (node->kind == ND_NE) {
		return function_def(operation(lhs, rhs, "!="));
	} else if (node->kind == ND_LT) {
		return function_def(operation(lhs, rhs, "<"));
	} else if (node->kind == ND_LE) {
		return function_def(operation(lhs, rhs, "<="));
	}

	error("invalid expression");
}

int gen_stmt(Node *node) {
	if (node->kind == ND_EXPR_STMT) {
		return gen_expr(node->lhs);
	}

	error("invalid statement");
}

void codegen(Node *node) {
	char *mainline = calloc(MAX_STRING, sizeof(char));
	strcpy(mainline, "int main() { int ret; ");

	Node *n;
	int nfunc;
	for (n = node; n; n = n->next) {
		strcat(mainline, "ret = f");
		nfunc = gen_stmt(n);
		strcat(mainline, uint2str(nfunc));
		strcat(mainline, "(); ");
	}

	strcat(mainline, "return ret; }\n");
	fputs(mainline, stdout);
}
