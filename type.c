#include "chibicc.h"

Type *ty_int;

void initialize_types(void) {
	ty_int = calloc(1, sizeof(Type));
	ty_int->kind = TY_INT;
}

int is_integer(Type *ty) {
	return ty->kind == TY_INT;
}

Type *pointer_to(Type *base) {
	Type *ty = calloc(1, sizeof(Type));
	ty->kind = TY_PTR;
	ty->base = base;
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

	if (node->kind == ND_ADD ||
			node->kind == ND_SUB ||
			node->kind == ND_MUL ||
			node->kind == ND_DIV ||
			node->kind == ND_NEG ||
			node->kind == ND_ASSIGN) {
		node->ty = node->lhs->ty;
	} else if (node->kind == ND_EQ ||
			node->kind == ND_NE ||
			node->kind == ND_LT ||
			node->kind == ND_LE ||
			node->kind == ND_VAR ||
			node->kind == ND_NUM) {
		node->ty = ty_int;
	} else if (node->kind == ND_ADDR) {
		node->ty = pointer_to(node->lhs->ty);
	} else if (node->kind == ND_DEREF) {
		if (node->lhs->ty->kind == TY_PTR) {
			node->ty = node->lhs->ty->base;
		} else {
			node->ty = ty_int;
		}
	}
}
