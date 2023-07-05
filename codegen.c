#include "chibicc.h"

int depth;

void push(void) {
	puts("push_eax");
	depth += 1;
}

void pop(char *arg) {
	if (strcmp(arg, "eax") || strcmp(arg, "ebx") || strcmp(arg, "ebp") || strcmp(arg, "edi")) {
		fputs("pop_", stdout);
		fputs(arg, stdout);
		fputc('\n', stdout);
	} else {
		error("invalid pop");
	}
	depth -= 1;
}

void gen_expr(Node *node) {
	// Sideless
	if (node->kind == ND_NUM) {
		fputs("mov_eax, %", stdout);
		fputs(uint2str(node->val), stdout);
		fputc('\n', stdout);
		return;
	}

	// Unary
	if (node->kind == ND_NEG) {
		gen_expr(node->lhs);
		puts("mov_ebx, %0");
		puts("sub_ebx,eax");
		puts("mov_eax,ebx");
		return;
	}

	// Binary
	gen_expr(node->rhs);
	push();
	gen_expr(node->lhs);

	if (node->kind == ND_ADD) {
		pop("ebx");
		puts("add_eax,ebx");
		return;
	} else if (node->kind == ND_SUB) {
		puts("mov_ebx,eax");
		pop("eax");
		puts("sub_ebx,eax");
		puts("mov_eax,ebx");
		return;
	} else if (node->kind == ND_MUL) {
		pop("ebx");
		puts("imul_ebx");
		return;
	} else if (node->kind == ND_DIV) {
		pop("ebx");
		puts("idiv_ebx");
		return;
	} else if (node->kind == ND_EQ || node->kind == ND_NE ||
			node->kind == ND_LT || node->kind == ND_LE) {
		puts("mov_ebx,eax");
		pop("eax");
		puts("cmp");
		if (node->kind == ND_EQ) {
			puts("sete_al");
		} else if (node->kind == ND_NE) {
			puts("setne_al");
		} else if (node->kind == ND_LT) {
			puts("setl_al");
		} else if (node->kind == ND_LE) {
			puts("setle_al");
		}
		puts("movzx_eax,al");
		return;
	}

	error("invalid expression");
}

void gen_stmt(Node *node) {
	if (node->kind == ND_EXPR_STMT) {
		gen_expr(node->lhs);
		return;
	}

	error("invalid statement");
}

void codegen(Node *node) {
	puts(":FUNCTION_main");

	Node *n;
	for (n = node; n; n = n->next) {
		gen_stmt(n);
		if (depth != 0) {
			error("depth not 0 at end of statement");
		}
	}

	puts("ret");
	puts(":ELF_data");
}
