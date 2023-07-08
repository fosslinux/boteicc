// This file contains a recursive descent parser for C.
//
// Most functions in this file are named after the symbols they are supposed
// to read from an input token list. For example, stmt() is responsible for
// reading a statement from a token list. The function then constructs an AST
// node representing a statement.
//
// Each function conceptually returns two values, an AST node and remaining
// part of the input tokens. Remaining tokens are returned to the caller via
// a pointer argument.
//
// Input tokens are represented by a linked list. Unlike many recursive
// descent parsers, we don't have the notion of the "input token stream". Most
// parsing functions don't change the global state of the parser, so it is
// very easy to lookahead arbitary number of tokens in this parser.

#include "chibicc.h"

// All variable instances created during parsing are accumulated
// to this list.
Obj *locals;
Obj *globals;

// Find a local variable by name.
Obj *find_var(Token *tok) {
	Obj *var;
	for (var = locals; var; var = var->next) {
		// If the variable is actually the variable
		if (strlen(var->name) == tok->len &&
				!strncmp(tok->loc, var->name, tok->len)) {
			return var;
		}
	}

	for (var = globals; var; var = var->next) {
		// If the variable is actually the variable
		if (strlen(var->name) == tok->len &&
				!strncmp(tok->loc, var->name, tok->len)) {
			return var;
		}
	}

	return NULL;
}

Node *new_node(int kind, Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->tok = tok;
	return node;
}

Node *new_binary(int kind, Node *lhs, Node *rhs, Token *tok) {
	Node *node = new_node(kind, tok);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_unary(int kind, Node *expr, Token *tok) {
	Node *node = new_node(kind, tok);
	node->lhs = expr;
	return node;
}

Node *new_num(int val, Token *tok) {
	Node *node = new_node(ND_NUM, tok);
	node->val = val;
	return node;
}

Node *new_var_node(Obj *var, Token *tok) {
	Node *node = new_node(ND_VAR, tok);
	node->var = var;
	return node;
}

Obj *new_var(char *name, Type *ty) {
	Obj *var = calloc(1, sizeof(Obj));
	var->name = name;
	var->ty = ty;
	return var;
}

Obj *new_lvar(char *name, Type *ty) {
	Obj *var = new_var(name, ty);
	var->is_local = TRUE;
	var->next = locals;
	locals = var;
	return var;
}

Obj *new_gvar(char *name, Type *ty) {
	Obj *var = new_var(name, ty);
	var->next = globals;
	globals = var;
	return var;
}

char *get_ident(Token *tok) {
	if (tok->kind != TK_IDENT) {
		error_tok(tok, "expected an identifier");
	}
	char *new = calloc(tok->len, sizeof(char));
	strncpy(new, tok->loc, tok->len);
	return new;
}

Type *declspec(Token **rest, Token *tok);
Type *declarator(Token **rest, Token *tok, Type *ty);
Node *declaration(Token **rest, Token *tok);
Node *compound_stmt(Token **rest, Token *tok);
Node *stmt(Token **rest, Token *tok);
Node *expr_stmt(Token **rest, Token *tok);
Node *expr(Token **rest, Token *tok);
Node *assign(Token **rest, Token *tok);
Node *equality(Token **rest, Token *tok);
Node *relational(Token **rest, Token *tok);
Node *add(Token **rest, Token *tok);
Node *mul(Token **rest, Token *tok);
Node *postfix(Token **rest, Token *tok);
Node *unary(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);

// declspec = "int"
Type *declspec(Token **rest, Token *tok) {
	*rest = skip(tok, "int");
	return ty_int;
}

// func-params = (param ("," param)*)? ")"
// param       = declspec declarator
Type *func_params(Token **rest, Token *tok, Type *ty) {
	Type *head = calloc(1, sizeof(Type));
	Type *cur = head;

	Type *basety;
	Type *nextty;
	while (!equal(tok, ")")) {
		if (cur != head) {
			tok = skip(tok, ",");
		}
		basety = declspec(&tok, tok);
		nextty = declarator(&tok, tok, basety);
		cur->next = copy_type(nextty);
		cur = cur->next;
	}

	ty = func_type(ty);
	ty->params = head->next;
	*rest = tok->next;
	return ty;
}

// type-suffix = "(" func-params
//             | "[" num "]" type-suffix
//             | ε
Type *type_suffix(Token **rest, Token *tok, Type *ty) {
	if (equal(tok, "(")) {
		return func_params(rest, tok->next, ty);
	}

	if (equal(tok, "[")) {
		int sz = get_number(tok->next);
		tok = skip(tok->next->next, "]");
		ty = type_suffix(rest, tok, ty);
		return array_of(ty, sz);
	}

	*rest = tok;
	return ty;
}

// declarator = "*"* ident type-suffix
Type *declarator(Token **rest, Token *tok, Type *ty) {
	while (consume(&tok, tok, "*")) {
		ty = pointer_to(ty);
	}

	if (tok->kind != TK_IDENT) {
		error_tok(tok, "expected a variable name");
	}

	ty = type_suffix(rest, tok->next, ty);
	ty->name = tok;
	return ty;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
Node *declaration(Token **rest, Token *tok) {
	Type *basety = declspec(&tok, tok);

	Node *head = calloc(1, sizeof(Node));
	Node *cur = head;
	int i = 0;

	Type *ty;
	Obj *var;
	Node *lhs;
	Node *rhs;
	Node *node;
	while (!equal(tok, ";")) {
		if (i > 0) {
			tok = skip(tok, ",");
		}
		i += 1;

		ty = declarator(&tok, tok, basety);
		var = new_lvar(get_ident(ty->name), ty);

		if (!equal(tok, "=")) {
			continue;
		}

		lhs = new_var_node(var, ty->name);
		rhs = assign(&tok, tok->next);
		node = new_binary(ND_ASSIGN, lhs, rhs, tok);
		cur->next = new_unary(ND_EXPR_STMT, node, tok);
		cur = cur->next;
	}

	node = new_node(ND_BLOCK, tok);
	node->body = head->next;
	*rest = tok->next;
	return node;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compound-stmt
//      | expr-stmt
Node *stmt(Token **rest, Token *tok) {
	if (equal(tok, "return")) {
		Node *node = new_node(ND_RETURN, tok);
		node->lhs = expr(&tok, tok->next);
		*rest = skip(tok, ";");
		return node;
	}

	if (equal(tok, "if")) {
		Node *node = new_node(ND_IF, tok);
		tok = skip(tok->next, "(");
		node->cond = expr(&tok, tok);
		tok = skip(tok, ")");
		node->then = stmt(&tok, tok);
		if (equal(tok, "else")) {
			node->els = stmt(&tok, tok->next);
		}
		*rest = tok;
		return node;
	}

	if (equal(tok, "for")) {
		Node *node = new_node(ND_FOR, tok);
		tok = skip(tok->next, "(");

		node->init = expr_stmt(&tok, tok);

		if (!equal(tok, ";")) {
			node->cond = expr(&tok, tok);
		}
		tok = skip(tok, ";");

		if (!equal(tok, ")")) {
			node->inc = expr(&tok, tok);
		}
		tok = skip(tok, ")");

		node->then = stmt(rest, tok);
		return node;
	}

	if (equal(tok, "while")) {
		Node *node = new_node(ND_FOR, tok);
		tok = skip(tok->next, "(");
		node->cond = expr(&tok, tok);
		tok = skip(tok, ")");
		node->then = stmt(rest, tok);
		return node;
	}

	if (equal(tok, "{")) {
		return compound_stmt(rest, tok->next);
	}

	return expr_stmt(rest, tok);
}

// compound-stmt = (declaration | stmt)* "}"
Node *compound_stmt(Token **rest, Token *tok) {
	Node *head = calloc(1, sizeof(Node));
	Node *cur = head;
	while (!equal(tok, "}")) {
		if (equal(tok, "int")) {
			cur->next = declaration(&tok, tok);
		} else {
			cur->next = stmt(&tok, tok);
		}
		cur = cur->next;
		add_type(cur);
	}

	Node *node = new_node(ND_BLOCK, tok);
	node->body = head->next;
	*rest = tok->next;
	return node;
}

// expr-stmt = expr? ";"
Node *expr_stmt(Token **rest, Token *tok) {
	if (equal(tok, ";")) {
		// Null statement
		*rest = tok->next;
		return new_node(ND_BLOCK, tok);
	}

	Node *node = new_node(ND_EXPR_STMT, tok);
	node->lhs = expr(&tok, tok);
	*rest = skip(tok, ";");
	return node;
}

// expr = assign
Node *expr(Token **rest, Token *tok) {
	return assign(rest, tok);
}

// assign = equality ("=" assign)?
Node *assign(Token **rest, Token *tok) {
	Node *node = equality(&tok, tok);
	if (equal(tok, "=")) {
		node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next), tok);
	}
	*rest = tok;
	return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(Token **rest, Token *tok) {
	Node *node = relational(&tok, tok);

	Token *start;
	while (1) {
		start = tok;
		if (equal(tok, "==")) {
			node = new_binary(ND_EQ, node, relational(&tok, tok->next), start);
			continue;
		}
		
		if (equal(tok, "!=")) {
			node = new_binary(ND_NE, node, relational(&tok, tok->next), start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(Token **rest, Token *tok) {
	Node *node = add(&tok, tok);

	Token *start;
	while (1) {
		start = tok;
		if (equal(tok, "<")) {
			node = new_binary(ND_LT, node, add(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, "<=")) {
			node = new_binary(ND_LE, node, add(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, ">")) {
			node = new_binary(ND_LT, add(&tok, tok->next), node, start);
			continue;
		}

		if (equal(tok, ">=")) {
			node = new_binary(ND_LE, add(&tok, tok->next), node, start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

// TODO: M2-Planet bug where we cannot && some of the conditions below.
// I can't be bothered to figure out which ones do it when, because it only
// occurs under weird conditions.
// So in these two functions we don't use &&.

// In C, `+` operator is overloaded to perform pointer arithmetic.
// If p is a pointer, p+n does not add n as usual, but adds sizeof(*p)*n to
// the value of p. In effect this makes p+n point to the location n elements,
// not bytes, ahead of p.
// This function takes care of the scaling required.
Node *new_add(Node *lhs, Node *rhs, Token *tok) {
	add_type(lhs);
	add_type(rhs);

	// num + num
	if (is_integer(lhs->ty)) {
		if (is_integer(rhs->ty)) {
			return new_binary(ND_ADD, lhs, rhs, tok);
		}
	}

	// ptr + ptr
	if (lhs->ty->base) {
		if (rhs->ty->base) {
			error_tok(tok, "invalid operands");
		}
	}

	// Canonicalize `num + ptr` to `ptr + num`.
	if (!lhs->ty->base) {
		if (rhs->ty->base) {
			Node *tmp = lhs;
			lhs = rhs;
			rhs = tmp;
		}
	}

	// ptr + num
	rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
	return new_binary(ND_ADD, lhs, rhs, tok);
}

// Like `+`, `-` is overloaded for the pointer type.
Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
	add_type(lhs);
	add_type(rhs);

	// num - num
	if (is_integer(lhs->ty)) {
		if (is_integer(rhs->ty)) {
			return new_binary(ND_SUB, lhs, rhs, tok);
		}
	}

	// ptr - num
	if (lhs->ty->base) {
		if (is_integer(rhs->ty)) {
    		rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
    		add_type(rhs);
    		Node *node = new_binary(ND_SUB, lhs, rhs, tok);
    		node->ty = lhs->ty;
    		return node;
		}
	}

	// ptr - ptr, which returns how many elements are between the two.
	if (lhs->ty->base) {
		if (rhs->ty->base) {
			Node *node = new_binary(ND_SUB, lhs, rhs, tok);
			node->ty = ty_int;
			return new_binary(ND_DIV, node, new_num(lhs->ty->base->size, tok), tok);
		}
	}

	error_tok(tok, "invalid operands");
}

// add = mul ("+" mul | "-" mul)*
Node *add(Token **rest, Token *tok) {
	Node *node = mul(&tok, tok);

	Token *start;
	while (1) {
		start = tok;
		if (equal(tok, "+")) {
			node = new_add(node, mul(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, "-")) {
			node = new_sub(node, mul(&tok, tok->next), start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

// mul = unary ("*" unary | "/" unary)*
Node *mul(Token **rest, Token *tok) {
	Node *node = unary(&tok, tok);

	Token *start;
	while (1) {
		start = tok;
		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, unary(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, "/")) {
			node = new_binary(ND_DIV, node, unary(&tok, tok->next), start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

// unary = ("+" | "-" | "*" | "&" ) unary
//       | postfix
Node *unary(Token **rest, Token *tok) {
	if (equal(tok, "+")) {
		return unary(rest, tok->next);
	}

	if (equal(tok, "-")) {
		return new_unary(ND_NEG, unary(rest, tok->next), tok);
	}

	if (equal(tok, "&")) {
		return new_unary(ND_ADDR, unary(rest, tok->next), tok);
	}

	if (equal(tok, "*")) {
		return new_unary(ND_DEREF, unary(rest, tok->next), tok);
	}

	return postfix(rest, tok);
}

// postfix = primary ("[" expr "]")*
Node *postfix(Token **rest, Token *tok) {
	Node *node = primary(&tok, tok);

	Token *start;
	Node *idx;
	while (equal(tok, "[")) {
		// x[y] => *(x+y)
		start = tok;
		idx = expr(&tok, tok->next);
		tok = skip(tok, "]");
		node = new_unary(ND_DEREF, new_add(node, idx, start), start);
	}

	*rest = tok;
	return node;
}

// funcall = ident "(" (assign ("," assign)*)? ")"
Node *funcall(Token **rest, Token *tok) {
	Token *start = tok;
	tok = tok->next->next;

	Node *head = calloc(1, sizeof(Node));
	Node *cur = head;

	while (!equal(tok, ")")) {
		if (cur != head) {
			tok = skip(tok, ",");
		}
		cur->next = assign(&tok, tok);
		cur = cur->next;
	}

	*rest = skip(tok, ")");

	Node *node = new_node(ND_FUNCALL, start);
	node->funcname = calloc(start->len, sizeof(char));
	strncpy(node->funcname, start->loc, start->len);
	node->args = head->next;
	return node;
}

// primary = "(" expr ")" | "sizeof" unary | ident func-args? | num
Node *primary(Token **rest, Token *tok) {
	if (equal(tok, "(")) {
		Node *node = expr(&tok, tok->next);
		*rest = skip(tok, ")");
		return node;
	}

	if (equal(tok, "sizeof")) {
		Node *node = unary(rest, tok->next);
		add_type(node);
		return new_num(node->ty->size, tok);
	}

	if (tok->kind == TK_IDENT) {
		// Function call
		if (equal(tok->next, "(")) {
			return funcall(rest, tok);
		}

		// Variable
		Obj *var = find_var(tok);
		// If variable not yet set
		if (!var) {
			error_tok(tok, "undefined variable");
		}
		*rest = tok->next;
		return new_var_node(var, tok);
	}

	if (tok->kind == TK_NUM) {
		Node *node = new_num(tok->val, tok);
		*rest = tok->next;
		return node;
	}

	error_tok(tok, "expected an expression");
}

void create_param_lvars(Type *param) {
	if (param) {
		create_param_lvars(param->next);
		new_lvar(get_ident(param->name), param);
	}
}

Token *function(Token *tok, Type *basety) {
	Type *ty = declarator(&tok, tok, basety);

	Obj *fn = new_gvar(get_ident(ty->name), ty);
	fn->is_function = TRUE;

	locals = NULL;
	create_param_lvars(ty->params);
	fn->params = locals;

	tok = skip(tok, "{");
	fn->body = compound_stmt(&tok, tok);
	fn->locals = locals;
	return tok;
}

Token *global_variable(Token *tok, Type *basety) {
	int first = TRUE;

	Type *ty;
	while (!consume(&tok, tok, ";")) {
		if (!first) {
			tok = skip(tok, ",");
		}
		first = FALSE;

		ty = declarator(&tok, tok, basety);
		new_gvar(get_ident(ty->name), ty);
	}

	return tok;
}

// Lookahead tokens, returning true if a given token is the start of a
// function definition or declaration.
int is_function(Token *tok) {
	if (equal(tok, ";")) {
		return FALSE;
	}

	Type *dummy = calloc(1, sizeof(Type));
	Type *ty = declarator(&tok, tok, dummy);
	return ty->kind == TY_FUNC;
}

// program = (function-definition | global-variable)*
Obj *parse(Token *tok) {
	globals = NULL;

	Type *basety;
	while (tok->kind != TK_EOF) {
		basety = declspec(&tok, tok);

		// Function
		if (is_function(tok)) {
			tok = function(tok, basety);
			continue;
		}

		// Global variable
		tok = global_variable(tok, basety);
	}
	return globals;
}
