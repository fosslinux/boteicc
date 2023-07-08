#include "chibicc.h"

int depth;
Obj *current_fn;

void gen_expr(Node *node);

int counter = 1;
int count(void) {
	counter += 1;
	return counter;
}

void push(char *arg) {
	if (strcmp(arg, "eax") || strcmp(arg, "ebx") || strcmp(arg, "ebp") || strcmp(arg, "edi")) {
		str_postfix("push_", arg);
	} else {
		error("invalid push");
	}
	depth += 1;
}

void pop(char *arg) {
	if (strcmp(arg, "eax") || strcmp(arg, "ebx") || strcmp(arg, "ebp") || strcmp(arg, "edi")) {
		str_postfix("pop_", arg);
	} else {
		error("invalid pop");
	}
	depth -= 1;
}

void expand_stack(int size) {
	num_postfix("sub_esp, %", size);
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
		if (node->var->is_local) {
			// Local variable
			str_postfix("lea_eax,[ebp+DWORD] %", int2str(node->var->offset, 10, TRUE));
		} else {
			// Global variable
			str_postfix("mov_eax, &GLOBAL_", node->var->name);
		}
	} else if (node->kind == ND_DEREF) {
		gen_expr(node->lhs);
	} else {
		error_tok(node->tok, "not an lvalue");
	}
}

// Load a value from an address in %eax.
void load(Type *ty) {
	if (ty->kind == TY_ARRAY) {
		// If it is an array, do not attempt to load a value into the register,
		// because in general, we cannot load an entire array into a register.
		// The result of evaluations from arrays becomes the address of the
		// array. This is the conversion from array => pointer to first element
		// of array occurs.
		return;
	}
	if (ty->size == 1) {
		puts("movsx_eax,BYTE_PTR_[eax]");
	} else {
		puts("mov_eax,[eax]");
	}
}

void mov_with_size(Type *ty) {
	if (ty->size == 1) {
		puts("mov_[ebx],al");
	} else {
		puts("mov_[ebx],eax");
	}
}

// Store %eax to the address in the top of the stack.
void store(Type *ty) {
	pop("ebx");
	mov_with_size(ty);
}

// Generate code for a given node.
void gen_expr(Node *node) {
	// Sideless
	if (node->kind == ND_NUM) {
		num_postfix("mov_eax, %", node->val);
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
		load(node->ty);
		return;
	} else if (node->kind == ND_DEREF) {
		gen_expr(node->lhs);
		load(node->ty);
		return;
	} else if (node->kind == ND_ADDR) {
		gen_addr(node->lhs);
		return;
	} else if (node->kind == ND_ASSIGN) {
		gen_addr(node->lhs);
		push("eax");
		gen_expr(node->rhs);
		store(node->ty);
		return;
	} else if (node->kind == ND_FUNCALL) {
		// We are using the cdecl calling convention.
		// Arguments are pushed onto the stack in right to left order.
		int narg = 0;
		Node *arg;
		for (arg = node->args; arg; arg = arg->next) {
			narg += 1;
		}

		int nargs = narg;

		int arg_counter = 0;
		while (narg != 0) {
			for (arg = node->args; arg_counter < narg - 1; arg = arg->next) {
				arg_counter += 1;
			}
			gen_expr(arg);
			puts("push_eax");

			narg -= 1;
			arg_counter = 0;
		}

		puts("mov_eax, %0");
		str_postfix("call %FUNCTION_", node->funcname);

		// Stack cleanup
		num_postfix("add_esp, %", nargs * 4);

		return;
	}

	// Binary
	gen_expr(node->rhs);
	push("eax");
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

	error_tok(node->tok, "invalid expression");
}

void gen_stmt(Node *node) {
	if (node->kind == ND_IF) {
		int c = count();
		gen_expr(node->cond);
		puts("mov_ebx, %0");
		puts("cmp");
		num_postfix("je %IF_else_", c);
		gen_stmt(node->then);
		num_postfix("jmp %IF_end_", c);
		num_postfix(":IF_else_", c);
		if (node->els) {
			gen_stmt(node->els);
		}
		num_postfix(":IF_end_", c);
		return;
	} else if (node->kind == ND_FOR) {
		int c = count();
		if (node->init) {
			gen_stmt(node->init);
		}
		num_postfix(":FOR_begin_", c);
		if (node->cond) {
			gen_expr(node->cond);
			puts("mov_ebx, %0");
			puts("cmp");
			num_postfix("je %FOR_end_", c);
		}
		gen_stmt(node->then);
		if (node->inc) {
			gen_expr(node->inc);
		}
		num_postfix("jmp %FOR_begin_", c);
		num_postfix(":FOR_end_", c);
		return;
	} else if (node->kind == ND_BLOCK) {
		Node *n;
		for (n = node->body; n; n = n->next) {
			gen_stmt(n);
		}
		return;
	} else if (node->kind == ND_RETURN) {
		gen_expr(node->lhs);
		str_postfix("jmp %BUILTIN_return_", current_fn->name);
		return;
	} else if (node->kind == ND_EXPR_STMT) {
		gen_expr(node->lhs);
		return;
	}

	error_tok(node->tok, "invalid statement");
}

// Assign offsets to local variables.
void assign_lvar_offsets(Obj *prog) {
	int offset;
	Obj *fn;
	Obj *var;
	for (fn = prog; fn; fn = fn->next) {
		if (!fn->is_function) {
			continue;
		}

		offset = 0;
		for (var = fn->locals; var; var = var->next) {
			offset += var->ty->size;
			var->offset = -offset;
		}
		fn->stack_size = align_to(offset, 16);
	}
}

void emit_data(Obj *prog) {
	puts(":ELF_data");

	Obj *var;
	int zero_count;
	int i;
	for (var = prog; var; var = var->next) {
		if (var->is_function) {
			continue;
		}
		str_postfix(":GLOBAL_", var->name);
		zero_count = var->ty->size;
		if (var->init_data) {
			for (i = 0; i < var->ty->size; i += 1) {
				fputc('!', stdout);
				fputs(uint2str(var->init_data[i]), stdout);
				fputc(' ', stdout);
			}
		} else {
			for (zero_count; zero_count > 0; zero_count -= 1) {
				fputs("!0 ", stdout);
			}
		}
		fputc('\n', stdout);
	}
}

void emit_text(Obj *prog) {
	Obj *fn;
	int i;
	Obj *var;
	for (fn = prog; fn; fn = fn->next) {
		if (!fn->is_function) {
			continue;
		}

		current_fn = fn;
		str_postfix(":FUNCTION_", fn->name);

		// Prologue
		puts("push_ebp");
		puts("mov_ebp,esp");
		expand_stack(fn->stack_size);

		// Save arguments to the stack.
		// They are already on the stack, but not at the offset we expect them
		// to be at.
		// We start at i = 2 because two things have been pushed onto the stack;
		// 1. return address by call
		// 2. push_ebp above
		i = 2;
		for (var = fn->params; var; var = var->next) {
			num_postfix("lea_eax,[ebp+DWORD] %", i * 4);
			puts("mov_edx,eax");
			str_postfix("lea_eax,[ebp+DWORD] %", int2str(var->offset, 10, TRUE));
			puts("mov_ebx,eax");
			puts("mov_eax,edx");
			puts("mov_eax,[eax]");
			mov_with_size(var->ty);
			i += 1;
		}

		// Emit code
		gen_stmt(fn->body);
		if (depth != 0) {
			error("depth not 0 at end of function");
		}

		// Epilogue
		str_postfix(":BUILTIN_return_", fn->name);
		puts("mov_esp,ebp");
		puts("pop_ebp");
		puts("ret");
	}
}

void codegen(Obj *prog) {
	assign_lvar_offsets(prog);
	emit_data(prog);
	emit_text(prog);
}
