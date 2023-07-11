#include "chibicc.h"

Type *ty_int;
Type *ty_char;

void initialize_types(void) {
	ty_int = calloc(1, sizeof(Type));
	ty_int->kind = TY_INT;
	ty_int->size = 8;

	ty_char = calloc(1, sizeof(Type));
	ty_char->kind = TY_CHAR;
	ty_char->size = 1;
}

int is_integer(Type *ty) {
	return ty->kind == TY_CHAR || ty->kind == TY_INT;
}

Type *copy_type(Type *ty) {
	Type *ret = calloc(1, sizeof(Type));
	ret->kind = ty->kind;
	ret->size = ty->size;
	ret->base = ty->base;
	ret->name = ty->name;
	ret->array_len = ty->array_len;
	ret->return_ty = ty->return_ty;
	ret->params = ty->params;
	ret->next = ty->next;
	return ret;
}

Type *pointer_to(Type *base) {
	Type *ty = calloc(1, sizeof(Type));
	ty->kind = TY_PTR;
	ty->size = 8;
	ty->base = base;
	return ty;
}

Type *func_type(Type *return_ty) {
	Type *ty = calloc(1, sizeof(Type));
	ty->kind = TY_FUNC;
	ty->return_ty = return_ty;
	return ty;
}

Type *array_of(Type *base, int len) {
	Type *ty = calloc(1, sizeof(Type));
	ty->kind = TY_ARRAY;
	ty->size = base->size * len;
	ty->base = base;
	ty->array_len = len;
	return ty;
}

// Adds typing information to a given node.
void add_type(Node *node) {
	if (!node) {
		return;
	}
	if (node->ty) {
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

	if (node->kind == ND_ADD ||
			node->kind == ND_SUB ||
			node->kind == ND_MUL ||
			node->kind == ND_DIV ||
			node->kind == ND_NEG) {
		node->ty = node->lhs->ty;
	} else if (node->kind == ND_ASSIGN) {
		if (node->lhs->ty->kind == TY_ARRAY) {
			error_tok(node->lhs->tok, "not an lvalue");
		}
		node->ty = node->lhs->ty;
	} else if (node->kind == ND_EQ ||
			node->kind == ND_NE ||
			node->kind == ND_LT ||
			node->kind == ND_LE ||
			node->kind == ND_NUM ||
			node->kind == ND_FUNCALL) {
		node->ty = ty_int;
	} else if (node->kind == ND_VAR) {
		node->ty = node->var->ty;
		return;
	} else if (node->kind == ND_COMMA) {
		node->ty = node->rhs->ty;
		return;
	} else if (node->kind == ND_MEMBER) {
		node->ty = node->member->ty;
		return;
	} else if (node->kind == ND_ADDR) {
		if (node->lhs->ty->kind == TY_ARRAY) {
			node->ty = pointer_to(node->lhs->ty->base);
		} else {
			node->ty = pointer_to(node->lhs->ty);
		}
	} else if (node->kind == ND_DEREF) {
		if (!node->lhs->ty->base) {
			error_tok(node->tok, "invalid pointer dereference");
		}
		node->ty = node->lhs->ty->base;
	} else if (node->kind == ND_STMT_EXPR) {
		if (node->body) {
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
		return;
	}
}
