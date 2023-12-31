#include "chibicc.h"

Type *ty_void;
Type *ty_bool;
Type *ty_char;
Type *ty_short;
Type *ty_int;
Type *ty_long;

Type *ty_uchar;
Type *ty_ushort;
Type *ty_uint;
Type *ty_ulong;

Type *new_type(int kind, int size, int align) {
	Type *ty = calloc(1, sizeof(Type));
	ty->kind = kind;
	ty->size = size;
	ty->align = align;
	return ty;
}

void initialize_types(void) {
	ty_void = new_type(TY_VOID, 1, 1);
	ty_bool = new_type(TY_BOOL, 1, 1);
	ty_char = new_type(TY_CHAR, 1, 1);
	ty_short = new_type(TY_SHORT, 2, 2);
	ty_int = new_type(TY_INT, 4, 4);
	ty_long = new_type(TY_LONG, 4, 4);

	ty_uchar = copy_type(ty_char);
	ty_uchar->is_unsigned = TRUE;
	ty_ushort = copy_type(ty_short);
	ty_ushort->is_unsigned = TRUE;
	ty_uint = copy_type(ty_int);
	ty_uint->is_unsigned = TRUE;
	ty_ulong = copy_type(ty_long);
	ty_ulong->is_unsigned = TRUE;
}

int is_integer(Type *ty) {
	int k = ty->kind;
	return k == TY_BOOL ||
		k == TY_CHAR ||
		k == TY_SHORT ||
		k == TY_INT ||
		k == TY_LONG ||
		k == TY_ENUM;
}

Type *copy_type(Type *t) {
	Type *ret = calloc(1, sizeof(Type));
	memcpy(ret, t, sizeof(Type));
	return ret;
}

Type *pointer_to(Type *base) {
	Type *ty = new_type(TY_PTR, 4, 4);
	ty->base = base;
	ty->is_unsigned = TRUE;
	return ty;
}

Type *func_type(Type *return_ty) {
	Type *ty = calloc(1, sizeof(Type));
	ty->kind = TY_FUNC;
	ty->return_ty = return_ty;
	return ty;
}

Type *array_of(Type *base, int len) {
	Type *ty = new_type(TY_ARRAY, base->size * len, base->align);
	ty->base = base;
	ty->array_len = len;
	return ty;
}

Type *enum_type(void) {
	return new_type(TY_ENUM, 4, 4);
}

Type *struct_type(void) {
	return new_type(TY_STRUCT, 0, 1);
}

Type *get_common_type(Type *ty1, Type *ty2) {
	if (ty1->base != NULL) {
		return pointer_to(ty1->base);
	}

	// Small integer promotion.
	if (ty1->size < 4) {
		ty1 = ty_int;
	}
	if (ty2->size < 4) {
		ty2 = ty_int;
	}

	if (ty1->kind == TY_LONG && ty2->kind == TY_INT) {
		return ty1;
	}

	if (ty2->kind == TY_LONG && ty1->kind == TY_INT) {
		return ty2;
	}

	if (ty2->is_unsigned) {
		return ty2;
	}
	return ty1;
}

// For many binary operators, we implicitly promote operands so that both
// operands have the same type.
// Eg, any integral type smaller than int is promoted to int.
//
// This is called the "usual arithmetic conversion".

// Adds typing information to a given node.
void add_type(Node *node) {
	if (node == NULL) {
		return;
	}
	if (node->ty != NULL) {
		return;
	}

	add_type(node->lhs);
	add_type(node->rhs);
	add_type(node->cond);
	add_type(node->then);
	add_type(node->els);
	add_type(node->init);
	add_type(node->inc);

	Node *n;
	for (n = node->body; n; n = n->next) {
		add_type(n);
	}
	for (n = node->args; n; n = n->next) {
		add_type(n);
	}

	if (node->kind == ND_NUM) {
		node->ty = ty_int;
	} else if (node->kind == ND_ADD ||
			node->kind == ND_SUB ||
			node->kind == ND_MUL ||
			node->kind == ND_DIV ||
			node->kind == ND_MOD ||
			node->kind == ND_BITAND ||
			node->kind == ND_BITOR ||
			node->kind == ND_BITXOR) {
		// usual arithmetic conversion
		Type *ty = get_common_type(node->lhs->ty, node->rhs->ty);
		node->lhs = new_cast(node->lhs, ty);
		node->rhs = new_cast(node->rhs, ty);
		node->ty = node->lhs->ty;
		return;
	} else if (node->kind == ND_NEG) {
		Type *ty = get_common_type(ty_int, node->lhs->ty);
		node->lhs = new_cast(node->lhs, ty);
		node->ty = ty;
	} else if (node->kind == ND_ASSIGN) {
		if (node->lhs->ty->kind == TY_ARRAY) {
			error_tok(node->lhs->tok, "not an lvalue");
		}
		if (node->lhs->ty->kind != TY_STRUCT) {
			node->rhs = new_cast(node->rhs, node->lhs->ty);
		}
		node->ty = node->lhs->ty;
	} else if (node->kind == ND_EQ ||
			node->kind == ND_NE ||
			node->kind == ND_LT ||
			node->kind == ND_LE) {
		// usual arithmetic conversion
		Type *ty = get_common_type(node->lhs->ty, node->rhs->ty);
		node->lhs = new_cast(node->lhs, ty);
		node->rhs = new_cast(node->rhs, ty);
		node->ty = ty_int;
	} else if (node->kind == ND_FUNCALL ||
			node->kind == ND_NOT ||
			node->kind == ND_LOGOR ||
			node->kind == ND_LOGAND) {
		node->ty = ty_int;
	} else if (node->kind == ND_BITNOT ||
			node->kind == ND_SHL ||
			node->kind == ND_SHR) {
		node->ty = node->lhs->ty;
	} else if (node->kind == ND_VAR) {
		node->ty = node->var->ty;
	} else if (node->kind == ND_COND) {
		if (node->then->ty->kind == TY_VOID || node->els->ty->kind == TY_VOID) {
			node->ty = ty_void;
		} else {
			// usual arithmetic conversion
			Type *ty = get_common_type(node->then->ty, node->els->ty);
			node->then = new_cast(node->then, ty);
			node->els = new_cast(node->els, ty);
			node->ty = node->then->ty;
		}
	} else if (node->kind == ND_COMMA) {
		node->ty = node->rhs->ty;
	} else if (node->kind == ND_MEMBER) {
		node->ty = node->member->ty;
	} else if (node->kind == ND_ADDR) {
		if (node->lhs->ty->kind == TY_ARRAY) {
			node->ty = pointer_to(node->lhs->ty->base);
		} else {
			node->ty = pointer_to(node->lhs->ty);
		}
	} else if (node->kind == ND_DEREF) {
		if (node->lhs->ty->base == NULL) {
			error_tok(node->tok, "invalid pointer dereference");
		}
		if (node->lhs->ty->base->kind == TY_VOID) {
			error_tok(node->tok, "dereferencing a void pointer");
		}
		node->ty = node->lhs->ty->base;
	} else if (node->kind == ND_STMT_EXPR) {
		if (node->body != NULL) {
			Node *stmt = node->body;
			while (stmt->next) {
				stmt = stmt->next;
			}
			if (stmt->kind == ND_EXPR_STMT) {
				node->ty = stmt->lhs->ty;
				return;
			}
		}
		error_tok(node->tok, "statement expression returning void is not supported");
	}
}
