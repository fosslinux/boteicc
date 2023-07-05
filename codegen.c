#include "chibicc.h"

int depth;

int counter = 1;
int count(void) {
	counter += 1;
	return counter;
}

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

// Round up `n` to the nearest multiple of `align.`
int align_to(int n, int align) {
	return (n + align - 1) / align * align;
}

// Compute the absolute address of a given node.
// TODO: Fix M2-Planet not liking apostrophes in comments..
// It is an error if the given node does not reside in memory.
void gen_addr(Node *node) {
	if (node->kind == ND_VAR) {
		fputs("lea_eax,[ebp+DWORD] %", stdout);
		fputs(int2str(node->var->offset, 10, TRUE), stdout);
		fputc('\n', stdout);
		return;
	}

	error("not an lvalue");
}

// Generate code for a given node.
void gen_expr(Node *node) {
	// Sideless
	if (node->kind == ND_NUM) {
		fputs("mov_eax, %", stdout);
		fputs(uint2str(node->val), stdout);
		fputc('\n', stdout);
		return;
	}

	if (node->kind == ND_NEG) {
		gen_expr(node->lhs);
		puts("mov_ebx, %0");
		puts("sub_ebx,eax");
		puts("mov_eax,ebx");
		return;
	} else if (node->kind == ND_VAR) {
		gen_addr(node);
		puts("mov_eax,[eax]");
		return;
	} else if (node->kind == ND_ASSIGN) {
		gen_addr(node->lhs);
		push();
		gen_expr(node->rhs);
		pop("ebx");
		puts("mov_[ebx],eax");
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
	if (node->kind == ND_IF) {
		int c = count();
		gen_expr(node->cond);
		puts("mov_ebx, %0");
		puts("cmp");
		fputs("je %COND_else_", stdout);
		fputs(uint2str(c), stdout);
		fputc('\n', stdout);
		gen_stmt(node->then);
		fputs("jmp %COND_end_", stdout);
		fputs(uint2str(c), stdout);
		fputc('\n', stdout);
		fputs(":COND_else_", stdout);
		fputs(uint2str(c), stdout);
		fputc('\n', stdout);
		if (node->els) {
			gen_stmt(node->els);
		}
		fputs(":COND_end_", stdout);
		fputs(uint2str(c), stdout);
		fputc('\n', stdout);
		return;
	} else if (node->kind == ND_BLOCK) {
		Node *n;
		for (n = node->body; n; n = n->next) {
			gen_stmt(n);
		}
		return;
	} else if (node->kind == ND_RETURN) {
		gen_expr(node->lhs);
		puts("jmp %BUILTIN_return");
		return;
	} else if (node->kind == ND_EXPR_STMT) {
		gen_expr(node->lhs);
		return;
	}

	error("invalid statement");
}

// Assign offsets to local variables.
void assign_lvar_offsets(Function *prog) {
	int offset = 0;
	Obj *var;
	for (var = prog->locals; var; var = var->next) {
		offset += 8;
		var->offset = -offset;
	}
	prog->stack_size = align_to(offset, 16);
}

void codegen(Function *prog) {
	assign_lvar_offsets(prog);

	puts(":FUNCTION_main");

	// Prologue
	puts("push_ebp");
	puts("mov_ebp,esp");
	fputs("sub_esp, %", stdout);
	fputs(uint2str(prog->stack_size), stdout);
	fputc('\n', stdout);

	gen_stmt(prog->body);
	if (depth != 0) {
		error("depth not 0 at end of statement");
	}

	// Epilogue
	puts(":BUILTIN_return");
	puts("mov_esp,ebp");
	pop("ebp");
	puts("ret");
	puts(":ELF_data");
}
