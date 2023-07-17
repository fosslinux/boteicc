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
// descent parsers, we do not have the notion of the "input token stream". Most
// parsing functions do not change the global state of the parser, so it is
// very easy to lookahead arbitary number of tokens in this parser.

#include "chibicc.h"

// Scope for local & global variables, typedefs, or enum constants.
struct VarScope {
	struct VarScope *next;
	char *name;
	Obj *var;
	Type *type_def;
	Type *enum_ty;
	int enum_val;
};
typedef struct VarScope VarScope;

// Scope for struct, union or enum tags.
struct TagScope {
	struct TagScope *next;
	char *name;
	Type *ty;
};
typedef struct TagScope TagScope;

// Represents a block scope.
struct Scope {
	struct Scope *next;

	// C has two block scopes; one is for variables, the other for tags.
	VarScope *vars;
	TagScope *tags;
};
typedef struct Scope Scope;

// Variable attributes (eg, typedef, extern);
struct VarAttr {
	int is_typedef;
	int is_static;
};
typedef struct VarAttr VarAttr;

Scope *scope;

// Points to the function currently being parsed.
Obj *parsing_fn;

// All variable instances created during parsing are accumulated
// to these lists respectively.
Obj *locals;
Obj *globals;

void enter_scope(void) {
	Scope *sc = calloc(1, sizeof(Scope));
	sc->next = scope;
	scope = sc;
}

void leave_scope(void) {
	scope = scope->next;
}

// Find a variable by name.
VarScope *find_var(Token *tok) {
	Obj *var;
	Scope *sc;
	VarScope *vsc;
	for (sc = scope; sc; sc = sc->next) {
		for (vsc = sc->vars; vsc; vsc = vsc->next) {
			if (equal(tok, vsc->name)) {
				return vsc;
			}
		}
	}
	return NULL;
}

Type *find_tag(Token *tok) {
	Scope *sc;
	TagScope *tsc;
	for (sc = scope; sc; sc = sc->next) {
		for (tsc = sc->tags; tsc; tsc = tsc->next) {
			if (equal(tok, tsc->name)) {
				return tsc->ty;
			}
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

Node *new_num(int32_t val, Token *tok) {
	Node *node = new_node(ND_NUM, tok);
	node->val = val;
	return node;
}

Node *new_var_node(Obj *var, Token *tok) {
	Node *node = new_node(ND_VAR, tok);
	node->var = var;
	return node;
}

Node *new_cast(Node *expr, Type *ty) {
	add_type(expr);

	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_CAST;
	node->tok = expr->tok;
	node->lhs = expr;
	node->ty = copy_type(ty);
	return node;
}

// Add a variable to the current scope.
VarScope *push_scope(char *name) {
	VarScope *sc = calloc(1, sizeof(VarScope));
	sc->name = name;
	sc->next = scope->vars;
	scope->vars = sc;
	return sc;
}

Obj *new_var(char *name, Type *ty) {
	Obj *var = calloc(1, sizeof(Obj));
	var->name = name;
	var->ty = ty;
	VarScope *vs = push_scope(name);
	vs->var = var;
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

int unique_id = 0;
char *new_unique_name(void) {
	unique_id += 1;
	return uint2str(unique_id);
}

Obj *new_anon_gvar(Type *ty) {
	return new_gvar(new_unique_name(), ty);
}

Obj *new_string_literal(char *p, Type *ty) {
	Obj *var = new_anon_gvar(ty);
	var->init_data = p;
	return var;
}

char *get_ident(Token *tok) {
	if (tok->kind != TK_IDENT) {
		error_tok(tok, "expected an identifier");
	}
	char *new = calloc(tok->len + 1, sizeof(char));
	strncpy(new, tok->loc, tok->len);
	return new;
}

void push_tag_scope(Token *tok, Type *ty) {
	TagScope *sc = calloc(1, sizeof(TagScope));
	sc->name = calloc(tok->len + 1, sizeof(char));
	strncpy(sc->name, tok->loc, tok->len);
	sc->ty = ty;
	sc->next = scope->tags;
	scope->tags = sc;
}

Type *find_typedef(Token *tok) {
	if (tok->kind == TK_IDENT) {
		VarScope *sc = find_var(tok);
		if (sc) {
			return sc->type_def;
		}
	}
	return NULL;
}

// Returns true if a given token represents a type.
int is_typename(Token *tok) {
	return equal(tok, "_Bool") ||
		equal(tok, "char") ||
		equal(tok, "short") ||
		equal(tok, "int") ||
		equal(tok, "long") ||
		equal(tok, "struct") ||
		equal(tok, "union") ||
		equal(tok, "void") ||
		equal(tok, "typedef") ||
		equal(tok, "enum") ||
		equal(tok, "static") ||
		find_typedef(tok);
}

Type *declspec(Token **rest, Token *tok, VarAttr *attr);
Type *enum_specifier(Token **rest, Token *tok);
Type *type_suffix(Token **rest, Token *tok, Type *ty);
Type *declarator(Token **rest, Token *tok, Type *ty);
Node *declaration(Token **rest, Token *tok, Type *basety);
Node *compound_stmt(Token **rest, Token *tok);
Node *stmt(Token **rest, Token *tok);
Node *expr_stmt(Token **rest, Token *tok);
Node *expr(Token **rest, Token *tok);
Node *assign(Token **rest, Token *tok);
Node *logor(Token **rest, Token *tok);
Node *logand(Token **rest, Token *tok);
Node *bitor(Token **rest, Token *tok);
Node *bitxor(Token **rest, Token *tok);
Node *bitand(Token **rest, Token *tok);
Node *equality(Token **rest, Token *tok);
Node *relational(Token **rest, Token *tok);
Node *add(Token **rest, Token *tok);
Node *new_add(Node *lhs, Node *rhs, Token *tok);
Node *new_sub(Node *lhs, Node *rhs, Token *tok);
Node *mul(Token **rest, Token *tok);
Node *cast(Token **rest, Token *tok);
Type *struct_decl(Token **rest, Token *tok);
Type *union_decl(Token **rest, Token *tok);
Node *postfix(Token **rest, Token *tok);
Node *unary(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);
Token *parse_typedef(Token *tok, Type *basety);

// declspec = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//          | "typedef" | "static"
//          | struct-decl | union-decl | typedef-name
//          | enum-specifier)+
//
// The order of typenames in a type specifier is irrelevant.
// Eg, `int long static` == `static long int` == `static long`.
// However, eg `char int` is not a valid type specifier.
// Only a limited combination of typenames is valid.
//
// In this function, we count the number of occurences of each typename, while
// keeping the current type object that the typenames until that point
// represent. Once a non-typename token is reached, we return the current type
// object.
Type *declspec(Token **rest, Token *tok, VarAttr *attr) {
	// We use a single integer as counters for all typenames.
	int counter = 0;

	int VOID   = 1 << 0;
	int BOOL   = 1 << 2;
	int CHAR   = 1 << 4;
	int SHORT  = 1 << 6;
	int INT    = 1 << 8;
	int LONG   = 1 << 10;
	int OTHER  = 1 << 12;

	Type *ty = ty_int; // Default
	
	Type *ty2;
	while (is_typename(tok)) {
		// Handle storage class specifiers.
		if (equal(tok, "typedef") || equal(tok, "static")) {
			if (!attr) {
				error_tok(tok, "storage class specifier is not allowed in this context");
			}
			if (equal(tok, "typedef")) {
				attr->is_typedef = TRUE;
			} else {
				attr->is_static = TRUE;
			}

			if (attr->is_typedef + attr->is_static > 1) {
				error_tok(tok, "typedef and static may not be used together");
			}
			tok = tok->next;
			continue;
		}

		// User-defined types.
		ty2 = find_typedef(tok);
		if (equal(tok, "struct") || equal(tok, "union") || equal(tok, "enum") || ty2) {
			if (counter) {
				break;
			}

			if (equal(tok, "struct")) {
				ty = struct_decl(&tok, tok->next);
			} else if (equal(tok, "union")) {
				ty = union_decl(&tok, tok->next);
			} else if (equal(tok, "enum")) {
				ty = enum_specifier(&tok, tok->next);
			} else {
				ty = ty2;
				tok = tok->next;
			}
			counter += OTHER;
			continue;
		}

		// Handle built-in types.
		if (equal(tok, "void")) {
			counter += VOID;
		} else if (equal(tok, "_Bool")) {
			counter += BOOL;
		} else if (equal(tok, "char")) {
			counter += CHAR;
		} else if (equal(tok, "short")) {
			counter += SHORT;
		} else if (equal(tok, "int")) {
			counter += INT;
		} else if (equal(tok, "long")) {
			counter += LONG;
		} else {
			error("internal error in declspec, invalid type");
		}

		if (counter == VOID) {
			ty = ty_void;
		} else if (counter == BOOL) {
			ty = ty_bool;
		} else if (counter == CHAR) {
			ty = ty_char;
		} else if (counter == SHORT || counter == SHORT + INT) {
			ty = ty_short;
		} else if (counter == INT) {
			ty = ty_int;
		} else if (counter == LONG || counter == LONG + INT) {
			ty = ty_long;
		} else {
			error_tok(tok, "invalid type");
		}

		tok = tok->next;
	}

	*rest = tok;
	return ty;
}

// func-params = (param ("," param)*)? ")"
// param       = declspec declarator
Type *func_params(Token **rest, Token *tok, Type *ty) {
	Type *head = calloc(1, sizeof(Type));
	Type *cur = head;

	Type *nextty;
	Token *name;
	while (!equal(tok, ")")) {
		if (cur != head) {
			tok = skip(tok, ",");
		}

		nextty = declspec(&tok, tok, NULL);
		nextty = declarator(&tok, tok, nextty);

		// "array of T" is converted to "pointer to T" onyl in the parameter
		// context. Eg, *argv[] => **argv.
		if (nextty->kind == TY_ARRAY) {
			name = nextty->name;
			nextty = pointer_to(nextty->base);
			nextty->name = name;
		}

		cur->next = copy_type(nextty);
		cur = cur->next;
	}

	ty = func_type(ty);
	ty->params = head->next;
	*rest = tok->next;
	return ty;
}

// array-dimensions = num? "]" type-suffix
Type *array_dimensions(Token **rest, Token *tok, Type *ty) {
	if (equal(tok, "]")) {
		ty = type_suffix(rest, tok->next, ty);
		return array_of(ty, -1);
	}

	int sz = get_number(tok);
	tok = skip(tok->next, "]");
	ty = type_suffix(rest, tok, ty);
	return array_of(ty, sz);
}

// type-suffix = "(" func-params
//             | "[" array-dimensions
//             | ε
Type *type_suffix(Token **rest, Token *tok, Type *ty) {
	if (equal(tok, "(")) {
		return func_params(rest, tok->next, ty);
	}

	if (equal(tok, "[")) {
		return array_dimensions(rest, tok->next, ty);
	}

	*rest = tok;
	return ty;
}

// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) type-suffix
Type *declarator(Token **rest, Token *tok, Type *ty) {
	while (consume(&tok, tok, "*")) {
		ty = pointer_to(ty);
	}

	if (equal(tok, "(")) {
		Token *start = tok;
		Type *dummy = calloc(1, sizeof(Type));
		declarator(&tok, start->next, dummy);
		tok = skip(tok, ")");
		ty = type_suffix(rest, tok, ty);
		return declarator(&tok, start->next, ty);
	}

	if (tok->kind != TK_IDENT) {
		error_tok(tok, "expected a variable name");
	}

	ty = type_suffix(rest, tok->next, ty);
	ty->name = tok;
	return ty;
}

// abstract-declarator = "*"* ("(" abstract-declarator ")")? type-suffix
Type *abstract_declarator(Token **rest, Token *tok, Type *ty) {
	while (equal(tok, "*")) {
		ty = pointer_to(ty);
		tok = tok->next;
	}

	if (equal(tok, "(")) {
		Token *start = tok;
		Type *dummy = calloc(1, sizeof(Type));
		abstract_declarator(&tok, start->next, dummy);
		tok = skip(tok, ")");
		ty = type_suffix(rest, tok, ty);
		return abstract_declarator(&tok, start->next, ty);
	}

	return type_suffix(rest, tok, ty);
}

// type-name = declspec abstract-declarator
Type *typename(Token **rest, Token *tok) {
	Type *ty = declspec(&tok, tok, NULL);
	return abstract_declarator(rest, tok, ty);
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident ("{" enum-list? "}")?
// enum-list      = ident ("=" num)? ("," ident ("=" num)?)*
Type *enum_specifier(Token **rest, Token *tok){
	Type *ty = enum_type();

	// Read an enum tag.
	Token *tag = NULL;
	if (tok->kind == TK_IDENT) {
		tag = tok;
		tok = tok->next;
	}

	// TODO another M2-Planet &&
	if (tag) {
		if (!equal(tok, "{")) {
			Type *ty = find_tag(tag);
			if (!ty) {
				error_tok(tag, "unknown enum type");
			}
			if (ty->kind != TY_ENUM) {
				error_tok(tag, "not an enum tag");
			}
			*rest = tok;
			return ty;
		}
	}

	tok = skip(tok, "{");

	// Read an enum-list.
	int i = 0;
	int val = 0;
	char *name;
	VarScope *sc;
	while (!equal(tok, "}")) {
		if (i > 0) {
			tok = skip(tok, ",");
		}
		i += 1;

		name = get_ident(tok);
		tok = tok->next;

		if (equal(tok, "=")) {
			val = get_number(tok->next);
			tok = tok->next->next;
		}

		sc = push_scope(name);
		sc->enum_ty = ty;
		sc->enum_val = val;
		val += 1;
	}

	*rest = tok->next;

	if (tag) {
		push_tag_scope(tag, ty);
	}
	return ty;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
Node *declaration(Token **rest, Token *tok, Type *basety) {
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
		if (ty->size < 0) {
			error_tok(tok, "variable has incomplete type");
		}
		if (ty->kind == TY_VOID) {
			error_tok(tok, "variable declared void");
		}

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
		Node *exp = expr(&tok, tok->next);
		*rest = skip(tok, ";");

		add_type(exp);
		node->lhs = new_cast(exp, parsing_fn->ty->return_ty);
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

		enter_scope();

		if (is_typename(tok)) {
			Type *basety = declspec(&tok, tok, NULL);
			node->init = declaration(&tok, tok, basety);
		} else {
			node->init = expr_stmt(&tok, tok);
		}

		if (!equal(tok, ";")) {
			node->cond = expr(&tok, tok);
		}
		tok = skip(tok, ";");

		if (!equal(tok, ")")) {
			node->inc = expr(&tok, tok);
		}
		tok = skip(tok, ")");

		node->then = stmt(rest, tok);
		leave_scope();
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

// compound-stmt = (typedef | declaration | stmt)* "}"
Node *compound_stmt(Token **rest, Token *tok) {
	Node *node = new_node(ND_BLOCK, tok);

	Node *head = calloc(1, sizeof(Node));
	Node *cur = head;

	enter_scope();

	VarAttr *attr;
	Type *basety;
	while (!equal(tok, "}")) {
		if (is_typename(tok)) {
			attr = calloc(1, sizeof(VarAttr));
			basety = declspec(&tok, tok, attr);

			if (attr->is_typedef) {
				tok = parse_typedef(tok, basety);
				continue;
			}

			cur->next = declaration(&tok, tok, basety);
		} else {
			cur->next = stmt(&tok, tok);
		}
		cur = cur->next;
		add_type(cur);
	}

	leave_scope();

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

// expr = assign ("," expr)?
Node *expr(Token **rest, Token *tok) {
	Node *node = assign(&tok, tok);

	if (equal(tok, ",")) {
		return new_binary(ND_COMMA, node, expr(rest, tok->next), tok);
	}

	*rest = tok;
	return node;
}

// Convert `A op= B` to `tmp = &A, *tmp = *tmp op B` where tmp is a
// fresh pointer variable.
Node *to_assign(Node *binary) {
	add_type(binary->lhs);
	add_type(binary->rhs);
	Token *tok = binary->tok;

	Obj *var = new_lvar("", pointer_to(binary->lhs->ty));

	Node *expr1 = new_binary(ND_ASSIGN, new_var_node(var, tok),
			new_unary(ND_ADDR, binary->lhs, tok), tok);

	Node *expr2 = new_binary(ND_ASSIGN,
			new_unary(ND_DEREF, new_var_node(var, tok), tok),
			new_binary(binary->kind,
				new_unary(ND_DEREF, new_var_node(var, tok), tok),
				binary->rhs,
				tok),
			tok);

	return new_binary(ND_COMMA, expr1, expr2, tok);
}

// assign    = logor (assign-op assign)?
// assign-op = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
Node *assign(Token **rest, Token *tok) {
	Node *node = logor(&tok, tok);

	if (equal(tok, "=")) {
		return new_binary(ND_ASSIGN, node, assign(rest, tok->next), tok);
	}

	if (equal(tok, "+=")) {
		return to_assign(new_add(node, assign(rest, tok->next), tok));
	}

	if (equal(tok, "-=")) {
		return to_assign(new_sub(node, assign(rest, tok->next), tok));
	}

	if (equal(tok, "*=")) {
		return to_assign(new_binary(ND_MUL, node, assign(rest, tok->next), tok));
	}

	if (equal(tok, "/=")) {
		return to_assign(new_binary(ND_DIV, node, assign(rest, tok->next), tok));
	}

	if (equal(tok, "%=")) {
		return to_assign(new_binary(ND_MOD, node, assign(rest, tok->next), tok));
	}

	if (equal(tok, "&=")) {
		return to_assign(new_binary(ND_BITAND, node, assign(rest, tok->next), tok));
	}

	if (equal(tok, "|=")) {
		return to_assign(new_binary(ND_BITOR, node, assign(rest, tok->next), tok));
	}

	if (equal(tok, "^=")) {
		return to_assign(new_binary(ND_BITXOR, node, assign(rest, tok->next), tok));
	}

	*rest = tok;
	return node;
}

// logor = logand ("||" logand)*
Node *logor(Token **rest, Token *tok) {
	Node *node = logand(&tok, tok);
	Token *start;
	while (equal(tok, "||")) {
		start = tok;
		node = new_binary(ND_LOGOR, node, logand(&tok, tok->next), start);
	}
	*rest = tok;
	return node;
}

// logand = bitor ("&&" bitor)*
Node *logand(Token **rest, Token *tok) {
	Node *node = bitor(&tok, tok);
	Token *start;
	while (equal(tok, "&&")) {
		start = tok;
		node = new_binary(ND_LOGAND, node, bitor(&tok, tok->next), start);
	}
	*rest = tok;
	return node;
}

// bitor = bitxor ("|" bitxor)*
Node *bitor(Token **rest, Token *tok) {
	Node *node = bitxor(&tok, tok);
	Token *start;
	while (equal(tok, "|")) {
		start = tok;
		node = new_binary(ND_BITOR, node, bitxor(&tok, tok->next), start);
	}
	*rest = tok;
	return node;
}

// bitxor = bitand ("^" bitand)*
Node *bitxor(Token **rest, Token *tok) {
	Node *node = bitand(&tok, tok);
	Token *start;
	while (equal(tok, "^")) {
		start = tok;
		node = new_binary(ND_BITXOR, node, bitand(&tok, tok->next), start);
	}
	*rest = tok;
	return node;
}

// bitand = equality ("&" equality)*
Node *bitand(Token **rest, Token *tok) {
	Node *node = equality(&tok, tok);
	Token *start;
	while (equal(tok, "&")) {
		start = tok;
		node = new_binary(ND_BITAND, node, equality(&tok, tok->next), start);
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
// I cannot be bothered to figure out which ones do it when, because it only
// occurs under weird conditions.
// So in these two functions we do not use &&.

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

// mul = cast ("*" cast | "/" cast | "%" cast)*
Node *mul(Token **rest, Token *tok) {
	Node *node = cast(&tok, tok);

	Token *start;
	while (1) {
		start = tok;
		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, cast(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, "/")) {
			node = new_binary(ND_DIV, node, cast(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, "%")) {
			node = new_binary(ND_MOD, node, cast(&tok, tok->next), start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

// cast = "(" type-nmae ")" cast | unary
Node *cast(Token **rest, Token *tok) {
	if (equal(tok, "(") && is_typename(tok->next)) {
		Token *start = tok;
		Type *ty = typename(&tok, tok->next);
		tok = skip(tok, ")");
		Node *node = new_cast(cast(rest, tok), ty);
		node->tok = start;
		return node;
	}

	return unary(rest, tok);
}

// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
Node *unary(Token **rest, Token *tok) {
	if (equal(tok, "+")) {
		return cast(rest, tok->next);
	}

	if (equal(tok, "-")) {
		return new_unary(ND_NEG, cast(rest, tok->next), tok);
	}

	if (equal(tok, "&")) {
		return new_unary(ND_ADDR, cast(rest, tok->next), tok);
	}

	if (equal(tok, "*")) {
		return new_unary(ND_DEREF, cast(rest, tok->next), tok);
	}

	if (equal(tok, "!")) {
		return new_unary(ND_NOT, cast(rest, tok->next), tok);
	}

	if (equal(tok, "~")) {
		return new_unary(ND_BITNOT, cast(rest, tok->next), tok);
	}

	// Read ++i as i+=1
	if (equal(tok, "++")) {
		return to_assign(new_add(unary(rest, tok->next), new_num(1, tok), tok));
	}

	// Read --i as i-=1
	if (equal(tok, "--")) {
		return to_assign(new_sub(unary(rest, tok->next), new_num(1, tok), tok));
	}

	return postfix(rest, tok);
}

// struct-members = (declspec declarator ("," declarator)* ";")*
void struct_members(Token **rest, Token *tok, Type *ty) {
	Member *head = calloc(1, sizeof(Member));
	Member *cur = head;

	Type *basety;
	Member *mem;
	int i;
	while (!equal(tok, "}")) {
		basety = declspec(&tok, tok, NULL);
		i = 0;

		while(!consume(&tok, tok, ";")) {
			if (i) {
				tok = skip(tok, ",");
			}
			i += 1;

			mem = calloc(1, sizeof(Member));
			mem->ty = declarator(&tok, tok, basety);
			mem->name = mem->ty->name;
			cur->next = mem;
			cur = cur->next;
		}
	}

	*rest = tok->next;
	ty->members = head->next;
}

// struct-union-decl = ident? ("{" struct-members)?
Type *struct_union_decl(Token **rest, Token *tok) {
	// Read a tag.
	Token *tag = NULL;
	if (tok->kind == TK_IDENT) {
		tag = tok;
		tok = tok->next;
	}

	if (tag != NULL && !equal(tok, "{")) {
		*rest = tok;

		Type *ty = find_tag(tag);
		if (ty) {
			return ty;
		}

		ty = struct_type();
		ty->size = -1;
		push_tag_scope(tag, ty);
		return ty;
	}

	tok = skip(tok, "{");

	// Construct the struct object.
	Type *ty = struct_type();
	struct_members(rest, tok, ty);

	if (tag) {
		// If this is a redefinition, overwrite the previous type; otherwise,
		// register the struct type.
		TagScope *sc;
		for (sc = scope->tags; sc; sc = sc->next) {
			if (equal(tag, sc->name)) {
				memcpy(sc->ty, ty, sizeof(Type));
				return sc->ty;
			}
		}

		push_tag_scope(tag, ty);
	}
	return ty;
}

// struct-decl = struct-union-decl
Type *struct_decl(Token **rest, Token *tok) {
	Type *ty = struct_union_decl(rest, tok);
	ty->kind = TY_STRUCT;

	if (ty->size < 0) {
		return ty;
	}

	// Assign offsets within the struct to members.
	int offset = 0;
	Member *mem;
	for (mem = ty->members; mem; mem = mem->next) {
		offset = align_to(offset, mem->ty->align);
		mem->offset = offset;
		offset += mem->ty->size;

		if (ty->align < mem->ty->align) {
			ty->align = mem->ty->align;
		}
	}
	ty->size = align_to(offset, ty->align);
	return ty;
}

// union-decl = struct-union-decl
Type *union_decl(Token **rest, Token *tok) {
	Type *ty = struct_union_decl(rest, tok);
	ty->kind = TY_UNION;

	if (ty->size < 0) {
		return ty;
	}

	// Offsets are not needed - they are already initialized to zero.
	// We need to compute alignment and size.
	Member *mem;
	for (mem = ty->members; mem; mem = mem->next) {
		if (ty->align < mem->ty->align) {
			ty->align = mem->ty->align;
		}
		if (ty->size < mem->ty->size) {
			ty->size = mem->ty->size;
		}
	}
	ty->size = align_to(ty->size, ty->align);
	return ty;
}

Member *get_struct_member(Type *ty, Token *tok) {
	Member *mem;
	for (mem = ty->members; mem; mem = mem->next) {
		if (mem->name->len == tok->len &&
				!strncmp(mem->name->loc, tok->loc, tok->len)) {
			return mem;
		}
	}
	error_tok(tok, "no such member");
}

Node *struct_ref(Node *lhs, Token *tok) {
	add_type(lhs);
	if (lhs->ty->kind != TY_STRUCT && lhs->ty->kind != TY_UNION) {
		error_tok(lhs->tok, "not a struct nor a union");
	}

	Node *node = new_unary(ND_MEMBER, lhs, tok);
	node->member = get_struct_member(lhs->ty, tok);
	return node;
}

// Convert A++ to `(typeof A)((A += 1) - 1)`
Node *new_inc_dec(Node *node, Token *tok, int addend) {
	add_type(node);
	return new_cast(new_add(to_assign(new_add(node, new_num(addend, tok), tok)),
				new_num(-addend, tok), tok), node->ty);
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident | "++" | "--")*
Node *postfix(Token **rest, Token *tok) {
	Node *node = primary(&tok, tok);

	Token *start;
	Node *idx;
	while (1) {
		if (equal(tok, "[")) {
			// x[y] => *(x+y)
			start = tok;
			idx = expr(&tok, tok->next);
			tok = skip(tok, "]");
			node = new_unary(ND_DEREF, new_add(node, idx, start), start);
			continue;
		}

		if (equal(tok, ".")) {
			node = struct_ref(node, tok->next);
			tok = tok->next->next;
			continue;
		}

		if (equal(tok, "->")) {
			// x->y => (*x).y
			node = new_unary(ND_DEREF, node, tok);
			node = struct_ref(node, tok->next);
			tok = tok->next->next;
			continue;
		}

		if (equal(tok, "++")) {
			node = new_inc_dec(node, tok, 1);
			tok = tok->next;
			continue;
		}

		if (equal(tok, "--")) {
			node = new_inc_dec(node, tok, -1);
			tok = tok->next;
			continue;
		}

		*rest = tok;
		return node;
	}
}

// funcall = ident "(" (assign ("," assign)*)? ")"
Node *funcall(Token **rest, Token *tok) {
	Token *start = tok;
	tok = tok->next->next;

	VarScope *sc = find_var(start);
	if (!sc) {
		error_tok(start, "implicit declaration of a function");
	}
	if (!sc->var || sc->var->ty->kind != TY_FUNC) {
		error_tok(start, "not a function");
	}

	Type *ty = sc->var->ty;
	Type *param_ty = ty->params;

	Node *head = calloc(1, sizeof(Node));
	Node *cur = head;

	Node *arg;
	while (!equal(tok, ")")) {
		if (cur != head) {
			tok = skip(tok, ",");
		}
		
		arg = assign(&tok, tok);
		add_type(arg);

		if (param_ty) {
			if (param_ty->kind == TY_STRUCT || param_ty->kind == TY_UNION) {
				error_tok(arg->tok, "passing struct or union not yet supported");
			}
			arg = new_cast(arg, param_ty);
			param_ty = param_ty->next;
		}

		cur->next = arg;
		cur = cur->next;
	}

	*rest = skip(tok, ")");

	Node *node = new_node(ND_FUNCALL, start);
	node->funcname = calloc(start->len + 1, sizeof(char));
	strncpy(node->funcname, start->loc, start->len);
	node->func_ty = ty;
	node->ty = ty->return_ty;
	node->args = head->next;
	return node;
}

// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | "sizeof" "(" type-name ")"
//         | "sizeof" unary
//         | "_TEST_ASSERT" "(" assign "," assign ")" ";"
//         | ident func-args?
//         | str
//         | num
Node *primary(Token **rest, Token *tok) {
	Token *start = tok;

	if (equal(tok, "(") && equal(tok->next, "{")) {
		// This is a GNU statement expression.
		Node *node = new_node(ND_STMT_EXPR, tok);
		Node *stmt = compound_stmt(&tok, tok->next->next);
		node->body = stmt->body;
		*rest = skip(tok, ")");
		return node;
	}

	if (equal(tok, "(")) {
		Node *node = expr(&tok, tok->next);
		*rest = skip(tok, ")");
		return node;
	}

	if (equal(tok, "sizeof") && equal(tok->next, "(") && is_typename(tok->next->next)) {
		Type *ty = typename(&tok, tok->next->next);
		*rest = skip(tok, ")");
		return new_num(ty->size, start);
	}

	if (equal(tok, "sizeof")) {
		Node *node = unary(rest, tok->next);
		add_type(node);
		return new_num(node->ty->size, tok);
	}

	if (equal(tok, "_TEST_ASSERT")) {
		Token *orig_tok = tok;
		tok = skip(tok->next, "(");
		Node *expected = assign(&tok, tok);
		Node *eval = assign(&tok, tok->next);

		int code_length = tok->loc - eval->tok->loc;
		char *code = calloc(code_length + 1, sizeof(char));
		strncpy(code, eval->tok->loc, code_length);
		Obj *code_str = new_string_literal(code, array_of(ty_char, code_length + 1));
		Node *code_node = new_var_node(code_str, orig_tok);

		Node *node = new_node(ND_FUNCALL, orig_tok);
		node->funcname = "_assert_failed";
		node->args = expected;
		node->args->next = eval;
		node->args->next->next = code_node;

		*rest = tok->next;
		return node;
	}

	if (tok->kind == TK_IDENT) {
		// Function call
		if (equal(tok->next, "(")) {
			return funcall(rest, tok);
		}

		// Variable or enum constant
		VarScope *sc = find_var(tok);
		// If variable not yet set
		if (!sc) {
			error_tok(tok, "undefined variable");
		}
		if (!sc->var && !sc->enum_ty) {
			error_tok(tok, "undefined variable");
		}

		Node *node;
		if (sc->var) {
			node = new_var_node(sc->var, tok);
		} else {
			node = new_num(sc->enum_val, tok);
		}

		*rest = tok->next;
		return node;
	}

	if (tok->kind == TK_STR) {
		Obj *var = new_string_literal(tok->str, tok->ty);
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

Token *parse_typedef(Token *tok, Type *basety) {
	int first = TRUE;

	Type *ty;
	VarScope *vs;
	while (!consume(&tok, tok, ";")) {
		if (!first) {
			tok = skip(tok, ",");
		}
		first = FALSE;

		ty = declarator(&tok, tok, basety);
		vs = push_scope(get_ident(ty->name));
		vs->type_def = ty;
	}
	return tok;
}

void create_param_lvars(Type *param) {
	if (param) {
		create_param_lvars(param->next);
		new_lvar(get_ident(param->name), param);
	}
}

Token *function(Token *tok, Type *basety, VarAttr *attr) {
	Type *ty = declarator(&tok, tok, basety);

	Obj *fn = new_gvar(get_ident(ty->name), ty);
	fn->is_function = TRUE;
	fn->is_definition = !consume(&tok, tok, ";");
	fn->is_static = attr->is_static;

	if (!fn->is_definition) {
		return tok;
	}

	parsing_fn = fn;
	locals = NULL;
	enter_scope();
	create_param_lvars(ty->params);
	fn->params = locals;

	tok = skip(tok, "{");
	fn->body = compound_stmt(&tok, tok);
	fn->locals = locals;
	leave_scope();
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

// program = (typedef | function-definition | global-variable)*
Obj *parse(Token *tok) {
	// Setup scope
	scope = calloc(1, sizeof(Scope));

	globals = NULL;

	Type *basety;
	VarAttr *attr;
	while (tok->kind != TK_EOF) {
		attr = calloc(1, sizeof(VarAttr));
		basety = declspec(&tok, tok, attr);

		// Typedef
		if (attr->is_typedef) {
			tok = parse_typedef(tok, basety);
			continue;
		}

		// Function
		if (is_function(tok)) {
			tok = function(tok, basety, attr);
			continue;
		}

		// Global variable
		tok = global_variable(tok, basety);
	}
	return globals;
}
