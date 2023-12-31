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
	int is_extern;
	int align;
};
typedef struct VarAttr VarAttr;

// This struct represents a variable initializer.
struct Initializer {
	struct Initializer *next;
	Type *ty;
	Token *tok;
	int is_flexible;

	// If it is not an aggregate type, and it has an initializer, this is
	// an initialization expression.
	Node *expr;

	// If it is an initializer for an aggregate type (array or struct, for eg),
	// this is the initializers for its children.
	struct Initializer **children;
};
typedef struct Initializer Initializer;

// For local variable initializer.
struct InitDesg {
	struct InitDesg *next;
	int idx;
	Member *member;
	Obj *var;
};
typedef struct InitDesg InitDesg;

Scope *scope;

// Points to the function currently being parsed.
Obj *parsing_fn;

// Lists of all goto statements and labels in the current function
Node *gotos;
Node *labels;

// All variable instances created during parsing are accumulated
// to these lists respectively.
Obj *locals;
Obj *globals;

// Current "break" and "continue" jump targets.
char *brk_label;
char *cont_label;

// Points to a node representing a switch if we are parsing a switch statement.
// Otherwise, NULL.
Node *current_switch;

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

Node *new_ulong(int32_t val, Token *tok) {
	Node *node = new_node(ND_NUM, tok);
	node->val = val;
	node->ty = ty_ulong;
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

Initializer *new_initializer(Type *ty, int is_flexible) {
	Initializer *init = calloc(1, sizeof(Initializer));
	init->ty = ty;

	if (ty->kind == TY_ARRAY) {
		if (is_flexible && ty->size < 0) {
			init->is_flexible = TRUE;
			return init;
		}

		init->children = calloc(ty->array_len, sizeof(Initializer*));
		int i;
		for (i = 0; i < ty->array_len; i += 1) {
			init->children[i] = new_initializer(ty->base, FALSE);
		}
	} else if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
		// Count the number of struct members.
		int len = 0;
		Member *mem;
		for (mem = ty->members; mem; mem = mem->next) {
			len += 1;
		}

		init->children = calloc(len, sizeof(Initializer*));

		Initializer *child;
		for (mem = ty->members; mem; mem = mem->next) {
			if (is_flexible && ty->is_flexible && mem->next == NULL) {
				child = calloc(1, sizeof(Initializer));
				child->ty = mem->ty;
				child->is_flexible = TRUE;
				init->children[mem->idx] = child;
			} else {
				init->children[mem->idx] = new_initializer(mem->ty, FALSE);
			}
		}
	}

	return init;
}

Obj *new_var(char *name, Type *ty) {
	Obj *var = calloc(1, sizeof(Obj));
	var->name = name;
	var->ty = ty;
	var->align = ty->align;
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
	var->is_static = TRUE;
	var->is_definition = TRUE;
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
		if (sc != NULL) {
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
		equal(tok, "extern") ||
		equal(tok, "signed") ||
		equal(tok, "unsigned") ||
		equal(tok, "const") ||
		equal(tok, "volatile") ||
		equal(tok, "auto") ||
		equal(tok, "register") ||
		equal(tok, "restrict") ||
		equal(tok, "__restrict") ||
		equal(tok, "__restrict__") ||
		equal(tok, "_Noreturn") ||
		find_typedef(tok) != NULL;
}

Type *declspec(Token **rest, Token *tok, VarAttr *attr);
Type *typename(Token **rest, Token *tok);
Type *enum_specifier(Token **rest, Token *tok);
Type *type_suffix(Token **rest, Token *tok, Type *ty);
Type *declarator(Token **rest, Token *tok, Type *ty);
Node *declaration(Token **rest, Token *tok, Type *basety, VarAttr *attr);
void initializer2(Token **rest, Token *tok, Initializer *init);
Initializer *initializer(Token **rest, Token *tok, Type *ty, Type **new_ty);
Node *lvar_initializer(Token **rest, Token *tok, Obj *var);
void gvar_initializer(Token **rest, Token *tok, Obj *var);
Node *compound_stmt(Token **rest, Token *tok);
Node *stmt(Token **rest, Token *tok);
Node *expr_stmt(Token **rest, Token *tok);
Node *expr(Token **rest, Token *tok);
int32_t eval(Node *node);
int32_t eval2(Node *node, Obj **var);
int32_t eval_rval(Node *node, Obj **var);
Node *assign(Token **rest, Token *tok);
Node *logor(Token **rest, Token *tok);
int32_t const_expr(Token **rest, Token *tok);
Node *conditional(Token **rest, Token *tok);
Node *logand(Token **rest, Token *tok);
Node *bitor(Token **rest, Token *tok);
Node *bitxor(Token **rest, Token *tok);
Node *bitand(Token **rest, Token *tok);
Node *equality(Token **rest, Token *tok);
Node *relational(Token **rest, Token *tok);
Node *shift(Token **rest, Token *tok);
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
int is_function(Token *tok);
Token *function(Token *tok, Type *basety, VarAttr *attr);
Token *global_variable(Token *tok, Type *basety, VarAttr *attr);

// declspec = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//          | "typedef" | "static" | "extern"
//          | "signed" | "unsigned"
//          | struct-decl | union-decl | typedef-name
//          | enum-specifier
//          | "const" | "volatile" | "auto" | "register" | "restrict"
//          | "__restrict" | "__restrict__" | "_Noreturn")+
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

	int VOID     = 1 << 0;
	int BOOL     = 1 << 2;
	int CHAR     = 1 << 4;
	int SHORT    = 1 << 6;
	int INT      = 1 << 8;
	int LONG     = 1 << 10;
	int OTHER    = 1 << 12;
	int SIGNED   = 1 << 13;
	int UNSIGNED = 1 << 14;

	Type *ty = ty_int; // Default

	Type *ty2;
	while (is_typename(tok)) {
		// Recognise but ignore these.
		if (consume(&tok, tok, "const") || consume(&tok, tok, "volatile") ||
				consume(&tok, tok, "auto") || consume(&tok, tok, "register") ||
				consume(&tok, tok, "restrict") || consume(&tok, tok, "__restrict") ||
				consume(&tok, tok, "__restrict__") || consume(&tok, tok, "_Noreturn")) {
			continue;
		}

		// Handle storage class specifiers.
		if (equal(tok, "typedef") || equal(tok, "static") || equal(tok, "extern")) {
			if (attr == NULL) {
				error_tok(tok, "storage class specifier is not allowed in this context");
			}
			if (equal(tok, "typedef")) {
				attr->is_typedef = TRUE;
			} else if (equal(tok, "static")) {
				attr->is_static = TRUE;
			} else if (equal(tok, "extern")) {
				attr->is_extern = TRUE;
			}

			if (attr->is_typedef && attr->is_static + attr->is_static > 1) {
				error_tok(tok, "typedef may not be used together with static or extern");
			}
			tok = tok->next;
			continue;
		}

		// User-defined types.
		ty2 = find_typedef(tok);
		if (equal(tok, "struct") || equal(tok, "union") || equal(tok, "enum") || ty2 != NULL) {
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
		} else if (equal(tok, "signed")) {
			counter |= SIGNED;
		} else if (equal(tok, "unsigned")) {
			counter |= UNSIGNED;
		} else {
			error("internal error in declspec, invalid type");
		}

		if (counter == VOID) {
			ty = ty_void;
		} else if (counter == BOOL) {
			ty = ty_bool;
		} else if (counter == CHAR ||
				counter == SIGNED + CHAR) {
			ty = ty_char;
		} else if (counter == UNSIGNED + CHAR) {
			ty = ty_uchar;
		} else if (counter == SHORT ||
				counter == SHORT + INT ||
				counter == SIGNED + SHORT ||
				counter == SIGNED + SHORT + INT) {
			ty = ty_short;
		} else if (counter == UNSIGNED + SHORT ||
				counter == UNSIGNED + SHORT + INT) {
			ty = ty_ushort;
		} else if (counter == INT ||
				counter == SIGNED ||
				counter == SIGNED + INT) {
			ty = ty_int;
		} else if (counter == UNSIGNED ||
				counter == UNSIGNED + INT) {
			ty = ty_uint;
		} else if (counter == LONG ||
				counter == LONG + INT ||
				counter == SIGNED + LONG ||
				counter == SIGNED + LONG + INT) {
			ty = ty_long;
		} else if (counter == UNSIGNED + LONG ||
				counter == UNSIGNED + LONG + INT) {
			ty = ty_ulong;
		} else {
			error_tok(tok, "invalid type");
		}

		tok = tok->next;
	}

	*rest = tok;
	return ty;
}

// func-params = ("void" | param ("," param)*)? ")"
// param       = declspec declarator
Type *func_params(Token **rest, Token *tok, Type *ty) {
	if (equal(tok, "void") && equal(tok->next, ")")) {
		*rest = tok->next->next;
		return func_type(ty);
	}

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

		// "array of T" is converted to "pointer to T" only in the parameter
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

// array-dimensions = ("static" | "restrict")* const-expr? "]" type-suffix
Type *array_dimensions(Token **rest, Token *tok, Type *ty) {
	while (equal(tok, "static") || equal(tok, "restrict")) {
		tok = tok->next;
	}

	if (equal(tok, "]")) {
		ty = type_suffix(rest, tok->next, ty);
		return array_of(ty, -1);
	}

	int sz = const_expr(&tok, tok);
	tok = skip(tok, "]");
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

// pointers = ("*" ("const" | "volatile" | "restrict")*)*
Type *pointers(Token **rest, Token *tok, Type *ty) {
	while (consume(&tok, tok, "*")) {
		ty = pointer_to(ty);
		while (equal(tok, "const") || equal(tok, "volatile") ||
				equal(tok, "restrict") || equal(tok, "__restrict") ||
				equal(tok, "__restrict__")) {
			tok = tok->next;
		}
	}
	*rest = tok;
	return ty;
}

// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) type-suffix
Type *declarator(Token **rest, Token *tok, Type *ty) {
	ty = pointers(&tok, tok, ty);

	if (equal(tok, "(")) {
		Token *start = tok;
		Type *dummy = calloc(1, sizeof(Type));
		declarator(&tok, start->next, dummy);
		tok = skip(tok, ")");
		ty = type_suffix(rest, tok, ty);
		return declarator(&tok, start->next, ty);
	}

	Token *name = NULL;
	Token *name_pos = tok;

	if (tok->kind == TK_IDENT) {
		name = tok;
		tok = tok->next;
	}

	ty = type_suffix(rest, tok, ty);
	ty->name = name;
	ty->name_pos = name_pos;
	return ty;
}

// abstract-declarator = "*"* ("(" abstract-declarator ")")? type-suffix
Type *abstract_declarator(Token **rest, Token *tok, Type *ty) {
	ty = pointers(&tok, tok, ty);

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

int expr_is_end(Token *tok) {
	return equal(tok, "}") || (equal(tok, ",") && equal(tok->next, "}"));
}

int expr_consume_end(Token **rest, Token *tok) {
	if (equal(tok, "}")) {
		*rest = tok->next;
		return TRUE;
	}

	if (equal(tok, ",") && equal(tok->next, "}")) {
		*rest = tok->next->next;
		return TRUE;
	}

	return FALSE;
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident ("{" enum-list? "}")?
// enum-list      = ident ("=" num)? ("," ident ("=" num)?)* ","?
Type *enum_specifier(Token **rest, Token *tok){
	Type *ty = enum_type();

	// Read an enum tag.
	Token *tag = NULL;
	if (tok->kind == TK_IDENT) {
		tag = tok;
		tok = tok->next;
	}

	if (tag != NULL && !equal(tok, "{")) {
		Type *ty = find_tag(tag);
		if (ty == NULL) {
			error_tok(tag, "unknown enum type");
		}
		if (ty->kind != TY_ENUM) {
			error_tok(tag, "not an enum tag");
		}
		*rest = tok;
		return ty;
	}

	tok = skip(tok, "{");

	// Read an enum-list.
	int i = 0;
	int val = 0;
	char *name;
	VarScope *sc;
	while (!expr_consume_end(rest, tok)) {
		if (i > 0) {
			tok = skip(tok, ",");
		}
		i += 1;

		name = get_ident(tok);
		tok = tok->next;

		if (equal(tok, "=")) {
			val = const_expr(&tok, tok->next);
		}

		sc = push_scope(name);
		sc->enum_ty = ty;
		sc->enum_val = val;
		val += 1;
	}

	if (tag != NULL) {
		push_tag_scope(tag, ty);
	}
	return ty;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
Node *declaration(Token **rest, Token *tok, Type *basety, VarAttr *attr) {
	Node *head = calloc(1, sizeof(Node));
	Node *cur = head;
	int i = 0;

	Type *ty;
	Obj *var;
	Node *expr;
	VarScope *vs;
	while (!equal(tok, ";")) {
		if (i > 0) {
			tok = skip(tok, ",");
		}
		i += 1;

		ty = declarator(&tok, tok, basety);
		if (ty->kind == TY_VOID) {
			error_tok(tok, "variable declared void");
		}

		if (ty->name == NULL) {
			error_tok(ty->name_pos, "variable name omitted");
		}

		if (attr != NULL) {
			if (attr->is_static) {
				// This is a static local variable.
				var = new_anon_gvar(ty);
				vs = push_scope(get_ident(ty->name));
				vs->var = var;
				if (equal(tok, "=")) {
					gvar_initializer(&tok, tok->next, var);
				}
				continue;
			}
		}

		var = new_lvar(get_ident(ty->name), ty);
		if (attr != NULL) {
			if (attr->align != 0) {
				var->align = attr->align;
			}
		}

		if (equal(tok, "=")) {
			expr = lvar_initializer(&tok, tok->next, var);
			cur->next = new_unary(ND_EXPR_STMT, expr, tok);
			cur = cur->next;
		}

		if (var->ty->size < 0) {
			error_tok(ty->name, "variable has incomplete type");
		}
		if (var->ty->kind == TY_VOID) {
			error_tok(ty->name, "variable declared void");
		}
	}

	Node *node = new_node(ND_BLOCK, tok);
	node->body = head->next;
	*rest = tok->next;
	return node;
}

Token *skip_excess_element(Token *tok) {
	if (equal(tok, "{")) {
		tok = skip_excess_element(tok->next);
		return skip(tok, "}");
	}

	assign(&tok, tok);
	return tok;
}

// string-initializer = string-literal
void string_initializer(Token **rest, Token *tok, Initializer *init) {
	if (init->is_flexible) {
		Initializer *new_init = new_initializer(array_of(init->ty->base, tok->ty->array_len), FALSE);
		memcpy(init, new_init, sizeof(Initializer));
	}

	int len = init->ty->array_len;
	if (tok->ty->array_len < len) {
		len = tok->ty->array_len;
	}
	int i;
	Initializer *child;
	for (i = 0; i < len; i += 1) {
		child = init->children[i];
		child->expr = new_num(tok->str[i], tok);
	}
	*rest = tok->next;
}

int count_array_init_elements(Token *tok, Type *ty) {
	Initializer *dummy = new_initializer(ty->base, FALSE);
	int i = 0;

	for (i; !expr_consume_end(&tok, tok); i += 1) {
		if (i > 0) {
			tok = skip(tok, ",");
		}
		initializer2(&tok, tok, dummy);
	}
	return i;
}

// array-initializer1 = "{" initializer ("," initializer)* ","? "}"
void array_initializer1(Token **rest, Token *tok, Initializer *init) {
	tok = skip(tok, "{");

	if (init->is_flexible) {
		int len = count_array_init_elements(tok, init->ty);
		Initializer *new_init = new_initializer(array_of(init->ty->base, len), FALSE);
		memcpy(init, new_init, sizeof(Initializer));
	}

	int i;
	for (i = 0; !expr_consume_end(rest, tok); i += 1) {
		if (i > 0) {
			tok = skip(tok, ",");
		}

		if (i < init->ty->array_len) {
			initializer2(&tok, tok, init->children[i]);
		} else {
			tok = skip_excess_element(tok);
		}
	}
}

// array-initializer2 = initializer ("," initializer)*
void array_initializer2(Token **rest, Token *tok, Initializer *init) {
	if (init->is_flexible) {
		int len = count_array_init_elements(tok, init->ty);
		Initializer *new_init = new_initializer(array_of(init->ty->base, len), FALSE);
		memcpy(init, new_init, sizeof(Initializer));
	}

	int i;
	for (i = 0; i < init->ty->array_len && !expr_is_end(tok); i += 1) {
		if (i > 0) {
			tok = skip(tok, ",");
		}

		initializer2(&tok, tok, init->children[i]);
	}

	*rest = tok;
}

// struct-initializer1 = "{" initializer ("," initializer)* ","? "}"
void struct_initializer1(Token **rest, Token *tok, Initializer *init) {
	tok = skip(tok, "{");

	Member *mem = init->ty->members;

	while (!expr_consume_end(rest, tok)) {
		if (mem != init->ty->members) {
			tok = skip(tok, ",");
		}

		if (mem != NULL) {
			initializer2(&tok, tok, init->children[mem->idx]);
			mem = mem->next;
		} else {
			tok = skip_excess_element(tok);
		}
	}
}

// struct-initializer2 = initializer ("," initializer)*
void struct_initializer2(Token **rest, Token *tok, Initializer *init) {
	int first = TRUE;

	Member *mem;
	for (mem = init->ty->members; mem != NULL && !expr_is_end(tok); mem = mem->next) {
		if (!first) {
			tok = skip(tok, ",");
		}
		first = FALSE;
		initializer2(&tok, tok, init->children[mem->idx]);
	}
	*rest = tok;
}

// union_initializer = ("{" initializer "}") | initializer
void union_initializer(Token **rest, Token *tok, Initializer *init) {
	// Unlike structs, union initializers take only one initializer,
	// initializing the first union member.
	if (equal(tok, "{")) {
		initializer2(&tok, tok->next, init->children[0]);
		consume(&tok, tok, ",");
		*rest = skip(tok, "}");
	} else {
		initializer2(rest, tok, init->children[0]);
	}
}

// initializer = string-initializer | array-initializer
//             | struct-initializer | union-initializer
//             | assign
void initializer2(Token **rest, Token *tok, Initializer *init) {
	if (init->ty->kind == TY_ARRAY) {
		if (tok->kind == TK_STR) {
			string_initializer(rest, tok, init);
		} else {
			if (equal(tok, "{")) {
				array_initializer1(rest, tok, init);
			} else {
				array_initializer2(rest, tok, init);
			}
		}
	} else if (init->ty->kind == TY_STRUCT) {
		if (equal(tok, "{")) {
			struct_initializer1(rest, tok, init);
			return;
		}

		// A struct can be initialized by another struct.
		// E.g. `struct T x = y;` where y is a struct T.
		// Handle that case first,
		Node *expr = assign(rest, tok);
		add_type(expr);
		if (expr->ty->kind == TY_STRUCT) {
			init->expr = expr;
		} else {
			struct_initializer2(rest, tok, init);
		}
	} else if (init->ty->kind == TY_UNION) {
		union_initializer(rest, tok, init);
	} else {
		if (equal(tok, "{")) {
			// An initializer for a scalar variable can be surrounded by
			// braces.
			initializer2(&tok, tok->next, init);
			*rest = skip(tok, "}");
		} else {
			init->expr = assign(rest, tok);
		}
	}
}

Type *copy_struct_type(Type *ty) {
	ty = copy_type(ty);

	Member *head = calloc(1, sizeof(Member));
	Member *cur = head;
	Member *mem;
	Member *m;
	for (mem = ty->members; mem != NULL; mem = mem->next) {
		m = calloc(1, sizeof(Member));
		memcpy(m, mem, sizeof(Member));
		cur->next = m;
		cur = m;
	}

	ty->members = head->next;
	return ty;
}

Initializer *initializer(Token **rest, Token *tok, Type *ty, Type **new_ty) {
	Initializer *init = new_initializer(ty, TRUE);
	initializer2(rest, tok, init);

	if ((ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->is_flexible) {
		ty = copy_struct_type(ty);

		Member *mem = ty->members;
		while (mem->next != NULL) {
			mem = mem->next;
		}
		Initializer *child = init->children[mem->idx];
		mem->ty = child->ty;
		ty->size += mem->ty->size;

		*new_ty = ty;
	} else {
		*new_ty = init->ty;
	}

	return init;
}

Node *init_desg_expr(InitDesg *desg, Token *tok) {
	if (desg->var != NULL) {
		return new_var_node(desg->var, tok);
	}

	if (desg->member != NULL) {
		Node *node = new_unary(ND_MEMBER, init_desg_expr(desg->next, tok), tok);
		node->member = desg->member;
		return node;
	}

	Node *lhs = init_desg_expr(desg->next, tok);
	Node *rhs = new_num(desg->idx, tok);
	return new_unary(ND_DEREF, new_add(lhs, rhs, tok), tok);
}

Node *create_lvar_init(Initializer *init, Type *ty, InitDesg *desg, Token *tok) {
	if (ty->kind == TY_ARRAY) {
		Node *node = new_node(ND_NULL_EXPR, tok);
		int i;
		InitDesg *desg2;
		Node *rhs;
		for (i = 0; i < ty->array_len; i += 1) {
			desg2 = calloc(1, sizeof(InitDesg));
			desg2->next = desg;
			desg2->idx = i;
			rhs = create_lvar_init(init->children[i], ty->base, desg2, tok);
			node = new_binary(ND_COMMA, node, rhs, tok);
		}
		return node;
	}

	if (ty->kind == TY_STRUCT && init->expr == NULL) {
		Node *node = new_node(ND_NULL_EXPR, tok);

		Member *mem;
		InitDesg *desg2;
		Node *rhs;
		for (mem = ty->members; mem; mem = mem->next) {
			desg2 = calloc(1, sizeof(InitDesg));
			desg2->next = desg;
			desg2->member = mem;
			rhs = create_lvar_init(init->children[mem->idx], mem->ty, desg2, tok);
			node = new_binary(ND_COMMA, node, rhs, tok);
		}
		return node;
	}

	if (ty->kind == TY_UNION) {
		InitDesg *desg2;
		desg2 = calloc(1, sizeof(InitDesg));
		desg2->next = desg;
		desg2->member = ty->members;
		return create_lvar_init(init->children[0], ty->members->ty, desg2, tok);
	}

	if (init->expr == NULL) {
		return new_node(ND_NULL_EXPR, tok);
	}

	Node *lhs = init_desg_expr(desg, tok);
	return new_binary(ND_ASSIGN, lhs, init->expr, tok);
}

// A variable definition with an initializer is a shorthand notation for a
// variable definition followed by assignments. This function generates
// assignment expressions for an initializer.
Node *lvar_initializer(Token **rest, Token *tok, Obj *var) {
	// XXX What the hell?
	Type **new_ty = calloc(1, sizeof(Type*));
	Initializer *init = initializer(rest, tok, var->ty, new_ty);
	var->ty = *new_ty;
	InitDesg *desg = calloc(1, sizeof(InitDesg));
	desg->next = NULL;
	desg->idx = 0;
	desg->member = NULL;
	desg->var = var;

	// If a partial initializer list is given, the standard requires
	// unspecified elements are set to 0.
	// The easiest solution is to zero the entire region before initializing
	// it with user-supplied values.
	Node *lhs = new_node(ND_MEMZERO, tok);
	lhs->var = var;

	Node *rhs = create_lvar_init(init, var->ty, desg, tok);
	return new_binary(ND_COMMA, lhs, rhs, tok);
}

void write_gvar_buf(int *buf, uint32_t val, int sz) {
	// Take bottom sz bytes.
	if (sz == 1) {
		*buf = val & 0xff;
	} else if (sz == 2) {
		*buf = val & 0xffff;
	} else if (sz == 4) {
		*buf = val;
	} else {
		error("internal error in write_gvar_buf, invalid sz");
	}
}

// This takes a char* for buf, despite buf being a int* everywhere else, since
// we are not writing to it, but simply performing arithmetic on it.
// And the arithmetic does not work for an int*.
// (This is due to M2-Planet shenanigans).

// XXX Ignore the GCC errors resulting from this, everything is fine.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
Relocation *write_gvar_data(Relocation *cur, Initializer *init, Type *ty, char *buf, int offset) {
	if (ty->kind == TY_ARRAY) {
		int sz = ty->base->size;
		int i;
		for (i = 0; i < ty->array_len; i += 1) {
			// XXX For some reason order of operations is a bit cracked in
			// M2-Planet for offset calc.
			cur = write_gvar_data(cur, init->children[i], ty->base, buf, offset + (sz * i));
		}
		return cur;
	}

	if (ty->kind == TY_STRUCT) {
		Member *mem;
		for (mem = ty->members; mem != NULL; mem = mem->next) {
			cur = write_gvar_data(cur, init->children[mem->idx], mem->ty, buf, offset + mem->offset);
		}
		return cur;
	}

	if (ty->kind == TY_UNION) {
		// Just the first of the union, like a struct.
		return write_gvar_data(cur, init->children[0], ty->members->ty, buf, offset);
	}

	if (init->expr == NULL) {
		return cur;
	}

	Obj *var = NULL;
	int32_t val = eval2(init->expr, &var);

	if (var == NULL) {
		write_gvar_buf(buf + offset, val, ty->size);
		return cur;
	}

	Relocation *rel = calloc(1, sizeof(Relocation));
	rel->offset = offset;
	rel->var = var;
	rel->addend = val;
	cur->next = rel;
	return cur->next;
}

// Initializers for global variables are evaluated at compile-time and embedded
// into ELF_data. We serialize Initializers into a flat byte array. It is a
// compile error if an initializer list contains a non-constant expression.
void gvar_initializer(Token **rest, Token *tok, Obj *var) {
	// XXX Again, why?
	Type **new_ty = calloc(1, sizeof(Type*));
	Initializer *init = initializer(rest, tok, var->ty, new_ty);
	var->ty = *new_ty;

	Relocation *head = calloc(1, sizeof(Relocation));
	// We should really be callocing size var->ty->size, but M2-Planet does not
	// comply.
	// We should also be using char* but that causes M2-Planet to segfault (why?)
	int *buf = calloc(1, 8);
	write_gvar_data(head, init, var->ty, buf, 0);
	var->init_data = buf;
	var->rel = head->next;
}

// Ok, the cursed pointer int* char* things are finished.
#pragma GCC diagnostic pop

// stmt = "return" expr? ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "switch" "(" expr ")" stmt
//      | "case" const-expr ":" stmt
//      | "default" ":" stmt
//      | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "do" stmt "while" "(" expr ")" ";"
//      | "goto" ident ";"
//      | "break" ";"
//      | "continue" ";"
//      | ident ":" stmt
//      | "{" compound-stmt
//      | expr-stmt
Node *stmt(Token **rest, Token *tok) {
	if (equal(tok, "return")) {
		Node *node = new_node(ND_RETURN, tok);
		if (consume(rest, tok->next, ";")) {
			return node;
		}

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

	if (equal(tok, "switch")) {
		Node *node = new_node(ND_SWITCH, tok);
		tok = skip(tok->next, "(");
		node->cond = expr(&tok, tok);
		tok = skip(tok, ")");

		Node *sw = current_switch;
		current_switch = node;

		char *brk = brk_label;
		node->brk_label = new_unique_name();
		brk_label = node->brk_label;

		node->then = stmt(rest, tok);

		current_switch = sw;
		brk_label = brk;
		return node;
	}

	if (equal(tok, "case")) {
		if (current_switch == NULL) {
			error_tok(tok, "stray case");
		}

		Node *node = new_node(ND_CASE, tok);
		int val = const_expr(&tok, tok->next);
		tok = skip(tok, ":");
		node->label = new_unique_name();
		node->lhs = stmt(rest, tok);
		node->val = val;
		node->case_next = current_switch->case_next;
		current_switch->case_next = node;
		return node;
	}

	if (equal(tok, "default")) {
		if (current_switch == NULL) {
			error_tok(tok, "stray default");
		}

		Node *node = new_node(ND_CASE, tok);
		tok = skip(tok->next, ":");
		node->label = new_unique_name();
		node->lhs = stmt(rest, tok);
		current_switch->default_case = node;
		return node;
	}

	if (equal(tok, "for")) {
		Node *node = new_node(ND_FOR, tok);
		tok = skip(tok->next, "(");

		enter_scope();

		char *brk = brk_label;
		char *cont = cont_label;
		node->brk_label = new_unique_name();
		brk_label = node->brk_label;
		node->cont_label = new_unique_name();
		cont_label = node->cont_label;

		if (is_typename(tok)) {
			Type *basety = declspec(&tok, tok, NULL);
			node->init = declaration(&tok, tok, basety, NULL);
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
		brk_label = brk;
		cont_label = cont;
		leave_scope();
		return node;
	}

	if (equal(tok, "while")) {
		Node *node = new_node(ND_FOR, tok);
		tok = skip(tok->next, "(");
		node->cond = expr(&tok, tok);
		tok = skip(tok, ")");

		char *brk = brk_label;
		char *cont = cont_label;
		node->brk_label = new_unique_name();
		brk_label = node->brk_label;
		node->cont_label = new_unique_name();
		cont_label = node->cont_label;

		node->then = stmt(rest, tok);
		brk_label = brk;
		return node;
	}

	if (equal(tok, "do")) {
		Node *node = new_node(ND_DO, tok);

		char *brk = brk_label;
		char *cont = cont_label;
		node->brk_label = new_unique_name();
		brk_label = node->brk_label;
		node->cont_label = new_unique_name();
		cont_label = node->cont_label;

		node->then = stmt(&tok, tok->next);

		brk_label = brk;
		cont_label = cont;

		tok = skip(tok, "while");
		tok = skip(tok, "(");
		node->cond = expr(&tok, tok);
		tok = skip(tok, ")");
		*rest = skip(tok, ";");
		return node;
	}

	if (equal(tok, "goto")) {
		Node *node = new_node(ND_GOTO, tok);
		node->label = get_ident(tok->next);
		node->goto_next = gotos;
		gotos = node;
		*rest = skip(tok->next->next, ";");
		return node;
	}

	if (equal(tok, "break")) {
		if (brk_label == NULL) {
			error_tok(tok, "stray break");
		}
		Node *node = new_node(ND_GOTO, tok);
		node->unique_label = brk_label;
		*rest = skip(tok->next, ";");
		return node;
	}

	if (equal(tok, "continue")) {
		if (cont_label == NULL) {
			error_tok(tok, "stray continue");
		}
		Node *node = new_node(ND_GOTO, tok);
		node->unique_label = cont_label;
		*rest = skip(tok->next, ";");
		return node;
	}

	if (tok->kind == TK_IDENT && equal(tok->next, ":")) {
		Node *node = new_node(ND_LABEL, tok);
		node->label = calloc(tok->len + 1, sizeof(char));
		strncpy(node->label, tok->loc, tok->len);
		node->unique_label = new_unique_name();
		node->lhs = stmt(rest, tok->next->next);
		node->goto_next = labels;
		labels = node;
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
		if (is_typename(tok) && !equal(tok->next, ":")) {
			attr = calloc(1, sizeof(VarAttr));
			basety = declspec(&tok, tok, attr);

			if (attr->is_typedef) {
				tok = parse_typedef(tok, basety);
				continue;
			}

			if (is_function(tok)) {
				tok = function(tok, basety, attr);
				continue;
			}

			if (attr->is_extern) {
				tok = global_variable(tok, basety, attr);
				continue;
			}

			cur->next = declaration(&tok, tok, basety, attr);
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

// Evaluate a given node as a constant expression.
//
// A constant expression is either a number, or ptr+n where ptr is a pointer
// to a global variable and n is a number. The pointer form is accepted as
// an initialization to a global variable.
int32_t eval(Node *node) {
	return eval2(node, NULL);
}

int32_t eval2(Node *node, Obj **var) {
	add_type(node);

	if (node->kind == ND_ADD) {
		return eval2(node->lhs, var) + eval(node->rhs);
	} else if (node->kind == ND_SUB) {
		return eval2(node->lhs, var) - eval(node->rhs);
	} else if (node->kind == ND_MUL) {
		return eval(node->lhs) * eval(node->rhs);
	} else if (node->kind == ND_DIV) {
		if (node->ty->is_unsigned) {
			uint32_t lhs = eval(node->lhs);
			return lhs / eval(node->rhs);
		} else {
			return eval(node->lhs) / eval(node->rhs);
		}
	} else if (node->kind == ND_NEG) {
		return -eval(node->lhs);
	} else if (node->kind == ND_MOD) {
		if (node->ty->is_unsigned) {
			uint32_t lhs = eval(node->lhs);
			return lhs % eval(node->rhs);
		} else {
			return eval(node->lhs) % eval(node->rhs);
		}
	} else if (node->kind == ND_BITAND) {
		return eval(node->lhs) & eval(node->rhs);
	} else if (node->kind == ND_BITOR) {
		return eval(node->lhs) | eval(node->rhs);
	} else if (node->kind == ND_BITXOR) {
		return eval(node->lhs) ^ eval(node->rhs);
	} else if (node->kind == ND_SHL) {
		return eval(node->lhs) << eval(node->rhs);
	} else if (node->kind == ND_SHR) {
		if (node->ty->is_unsigned && node->ty->size == 4) {
			uint32_t lhs = eval(node->lhs);
			return lhs >> eval(node->rhs);
		} else {
			return eval(node->lhs) >> eval(node->rhs);
		}
	} else if (node->kind == ND_EQ) {
		return eval(node->lhs) == eval(node->rhs);
	} else if (node->kind == ND_NE) {
		return eval(node->lhs) != eval(node->rhs);
	} else if (node->kind == ND_LT) {
		if (node->lhs->ty->is_unsigned) {
			uint32_t lhs = eval(node->lhs);
			return lhs < eval(node->rhs);
		} else {
			return eval(node->lhs) < eval(node->rhs);
		}
	} else if (node->kind == ND_LE) {
		if (node->lhs->ty->is_unsigned) {
			uint32_t lhs = eval(node->lhs);
			return lhs < eval(node->rhs);
		} else {
			return eval(node->lhs) <= eval(node->rhs);
		}
	} else if (node->kind == ND_COND) {
		if (eval(node->cond)) {
			return eval2(node->then, var);
		} else {
			return eval2(node->els, var);
		}
	} else if (node->kind == ND_COMMA) {
		return eval2(node->rhs, var);
	} else if (node->kind == ND_NOT) {
		return !eval(node->lhs);
	} else if (node->kind == ND_BITNOT) {
		return ~eval(node->lhs);
	} else if (node->kind == ND_LOGAND) {
		if (eval(node->lhs)) {
			if (eval(node->rhs)) {
				return TRUE;
			}
		}
		return FALSE;
	} else if (node->kind == ND_LOGOR) {
		if (eval(node->lhs)) {
			return TRUE;
		}
		if (eval(node->rhs)) {
			return TRUE;
		}
		return FALSE;
	} else if (node->kind == ND_CAST) {
		int32_t val = eval2(node->lhs, var);
		if (is_integer(node->ty)) {
			if (node->ty->size == 1) {
				return val & 0xff;
			} else if (node->ty->size == 2) {
				return val & 0xffff;
			}
		}
		return val;
	} else if (node->kind == ND_ADDR) {
		return eval_rval(node->lhs, var);
	} else if (node->kind == ND_MEMBER) {
		if (var == NULL) {
			error_tok(node->tok, "not a compile time constant");
		}
		if (node->ty->kind != TY_ARRAY) {
			error_tok(node->tok, "invalid initalizer");
		}
		return eval_rval(node->lhs, var) + node->member->offset;
	} else if (node->kind == ND_VAR) {
		if (var == NULL) {
			error_tok(node->tok, "not a compile time constant");
		}
		if (node->var->ty->kind != TY_ARRAY && node->var->ty->kind != TY_FUNC) {
			error_tok(node->tok, "invalid initializer");
		}
		*var = node->var;
		return 0;
	} else if (node->kind == ND_NUM) {
		return node->val;
	}

	error_tok(node->tok, "not a compile time constant");
}

int32_t eval_rval(Node *node, Obj **var) {
	if (node->kind == ND_VAR) {
		if (node->var->is_local) {
			error_tok(node->tok, "not a compile-time constant");
		}
		*var = node->var;
		return 0;
	} else if (node->kind == ND_DEREF) {
		return eval2(node->lhs, var);
	} else if (node->kind == ND_MEMBER) {
		return eval_rval(node->lhs, var) + node->member->offset;
	}

	error_tok(node->tok, "invalid initializer");
}

int32_t const_expr(Token **rest, Token *tok) {
	Node *node = conditional(rest, tok);
	return eval(node);
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

// assign    = conditional (assign-op assign)?
// assign-op = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
//           | "<<=" | ">>="
Node *assign(Token **rest, Token *tok) {
	Node *node = conditional(&tok, tok);

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

	if (equal(tok, "<<=")) {
		return to_assign(new_binary(ND_SHL, node, assign(rest, tok->next), tok));
	}

	if (equal(tok, ">>=")) {
		return to_assign(new_binary(ND_SHR, node, assign(rest, tok->next), tok));
	}

	*rest = tok;
	return node;
}

// conditional = logor ("?" expr ":" conditional)?
Node *conditional(Token **rest, Token *tok) {
	Node *cond = logor(&tok, tok);

	if (!equal(tok, "?")) {
		*rest = tok;
		return cond;
	}

	Node *node = new_node(ND_COND, tok);
	node->cond = cond;
	node->then = expr(&tok, tok->next);
	tok = skip(tok, ":");
	node->els = conditional(rest, tok);
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

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
Node *relational(Token **rest, Token *tok) {
	Node *node = shift(&tok, tok);

	Token *start;
	while (1) {
		start = tok;
		if (equal(tok, "<")) {
			node = new_binary(ND_LT, node, shift(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, "<=")) {
			node = new_binary(ND_LE, node, shift(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, ">")) {
			node = new_binary(ND_LT, shift(&tok, tok->next), node, start);
			continue;
		}

		if (equal(tok, ">=")) {
			node = new_binary(ND_LE, shift(&tok, tok->next), node, start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

// shift = add ("<<" add | ">>" add)*
Node *shift(Token **rest, Token *tok) {
	Node *node = add(&tok, tok);

	Token *start;
	while (1) {
		start = tok;

		if (equal(tok, "<<")) {
			node = new_binary(ND_SHL, node, add(&tok, tok->next), start);
			continue;
		}

		if (equal(tok, ">>")) {
			node = new_binary(ND_SHR, node, add(&tok, tok->next), start);
			continue;
		}

		*rest = tok;
		return node;
	}
}

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
	if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
		return new_binary(ND_ADD, lhs, rhs, tok);
	}

	// ptr + ptr
	if (lhs->ty->base != NULL && rhs->ty->base != NULL) {
		error_tok(tok, "invalid operands");
	}

	// Canonicalize `num + ptr` to `ptr + num`.
	if (lhs->ty->base == NULL && rhs->ty->base != NULL) {
		Node *tmp = lhs;
		lhs = rhs;
		rhs = tmp;
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
	if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
		return new_binary(ND_SUB, lhs, rhs, tok);
	}

	// ptr - num
	if (lhs->ty->base != NULL && is_integer(rhs->ty)) {
		rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
		add_type(rhs);
		Node *node = new_binary(ND_SUB, lhs, rhs, tok);
		node->ty = lhs->ty;
		return node;
	}

	// ptr - ptr, which returns how many elements are between the two.
	if (lhs->ty->base != NULL && rhs->ty->base != NULL) {
		Node *node = new_binary(ND_SUB, lhs, rhs, tok);
		node->ty = ty_long;
		return new_binary(ND_DIV, node, new_num(lhs->ty->base->size, tok), tok);
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

		// compound literal
		if (equal(tok, "{")) {
			return unary(rest, start);
		}

		// type cast
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
	int idx = 0;

	Type *basety;
	Member *mem;
	VarAttr *attr;
	int first;
	while (!equal(tok, "}")) {
		first = TRUE;
		attr = calloc(1, sizeof(VarAttr));
		basety = declspec(&tok, tok, attr);

		while (!consume(&tok, tok, ";")) {
			if (!first) {
				tok = skip(tok, ",");
			}
			first = FALSE;

			mem = calloc(1, sizeof(Member));
			mem->ty = declarator(&tok, tok, basety);
			mem->name = mem->ty->name;
			mem->idx = idx;
			idx += 1;
			if (attr->align != 0) {
				mem->align = attr->align;
			} else {
				mem->align = mem->ty->align;
			}
			cur->next = mem;
			cur = cur->next;
		}
	}

	// If the last element is an array with incomplete type, it is a "flexible
	// array member", behaving as if it were a zero-sized array.
	if (cur != head) {
		if (cur->ty->kind == TY_ARRAY && cur->ty->array_len < 0) {
			cur->ty = array_of(cur->ty->base, 0);
			ty->is_flexible = TRUE;
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
		if (ty != NULL) {
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

	if (tag != NULL) {
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
		offset = align_to(offset, mem->align);
		mem->offset = offset;
		offset += mem->ty->size;

		if (ty->align < mem->align) {
			ty->align = mem->align;
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
		if (ty->align < mem->align) {
			ty->align = mem->align;
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

// postfix = "(" type-name ")" "{" initializer-list "}"
//         | primary ("[" expr "]" | "." ident | "->" ident | "++" | "--")*
Node *postfix(Token **rest, Token *tok) {
	// Compound literal handling
	if (equal(tok, "(") && is_typename(tok->next)) {
		Token *start = tok;
		Type *ty = typename(&tok, tok->next);
		tok = skip(tok, ")");

		if (scope->next == NULL) {
			Obj *var = new_anon_gvar(ty);
			gvar_initializer(rest, tok, var);
			return new_var_node(var, start);
		}

		Obj *var = new_lvar("", ty);
		Node *lhs = lvar_initializer(rest, tok, var);
		Node *rhs = new_var_node(var, tok);
		return new_binary(ND_COMMA, lhs, rhs, start);
	}

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
	if (sc == NULL) {
		error_tok(start, "implicit declaration of a function");
	}
	if (sc->var == NULL || sc->var->ty->kind != TY_FUNC) {
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

		if (param_ty != NULL) {
			if (param_ty->kind == TY_STRUCT || param_ty->kind == TY_UNION) {
				error_tok(arg->tok, "passing struct or union not yet supported");
			}
			arg = new_cast(arg, param_ty);
			param_ty = param_ty->next;
		} else {
			error_tok(tok, "too many arguments");
		}

		cur->next = arg;
		cur = cur->next;
	}

	if (param_ty != NULL) {
		error_tok(tok, "too few arguments");
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
//         | "_Alignof" "(" type-name ")"
//         | "_Alignof" unary
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
		return new_ulong(ty->size, start);
	}

	if (equal(tok, "sizeof")) {
		Node *node = unary(rest, tok->next);
		add_type(node);
		return new_ulong(node->ty->size, tok);
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

	if (equal(tok, "_Alignof") && equal(tok->next, "(") && is_typename(tok->next->next)) {
		Type *ty = typename(&tok, tok->next->next);
		*rest = skip(tok, ")");
		return new_ulong(ty->align, tok);
	}

	if (equal(tok, "_Alignof")) {
		Node *node = unary(rest, tok->next);
		add_type(node);
		return new_ulong(node->ty->align, tok);
	}

	if (tok->kind == TK_IDENT) {
		// Function call
		if (equal(tok->next, "(")) {
			return funcall(rest, tok);
		}

		// Variable or enum constant
		VarScope *sc = find_var(tok);
		// If variable not yet set
		if (sc == NULL) {
			error_tok(tok, "undefined variable");
		}
		if (sc->var == NULL && sc->enum_ty == NULL) {
			error_tok(tok, "undefined variable");
		}

		Node *node;
		if (sc->var != NULL) {
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
		node->ty = tok->ty;
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
		if (ty->name == NULL) {
			error_tok(ty->name_pos, "typedef name omitted");
		}
		vs = push_scope(get_ident(ty->name));
		vs->type_def = ty;
	}
	return tok;
}

void create_param_lvars(Type *param) {
	if (param != NULL) {
		create_param_lvars(param->next);
		if (param->name == NULL) {
			error_tok(param->name_pos, "parameter name omitted");
		}
		new_lvar(get_ident(param->name), param);
	}
}

// This function matches gotos with labels.
//
// We cannot resolve gotos in the same way as functions, because gotos can
// refer to a label appearing later in the function.
// Thus, this occurs after we parse the entire function.
void resolve_goto_labels(void) {
	Node *x;
	Node *y;
	for (x = gotos; x; x = x->goto_next) {
		for (y = labels; y; y = y->goto_next) {
			if (!strcmp(x->label, y->label)) {
				x->unique_label = y->unique_label;
				break;
			}
		}

		if (x->unique_label == NULL) {
			error_tok(x->tok->next, "use of undeclared label");
		}
	}

	gotos = NULL;
	labels = NULL;
}

Token *function(Token *tok, Type *basety, VarAttr *attr) {
	Type *ty = declarator(&tok, tok, basety);
	if (ty->name == NULL) {
		error_tok(ty->name_pos, "function name omitted");
	}

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
	resolve_goto_labels();
	return tok;
}

Token *global_variable(Token *tok, Type *basety, VarAttr *attr) {
	int first = TRUE;

	Type *ty;
	Obj *var;
	while (!consume(&tok, tok, ";")) {
		if (!first) {
			tok = skip(tok, ",");
		}
		first = FALSE;

		ty = declarator(&tok, tok, basety);
		if (ty->name == NULL) {
			error_tok(ty->name_pos, "variable name omitted");
		}

		var = new_gvar(get_ident(ty->name), ty);
		var->is_definition = !attr->is_extern;
		var->is_static = attr->is_static;
		if (attr->align != 0) {
			var->align = attr->align;
		}

		if (equal(tok, "=")) {
			gvar_initializer(&tok, tok->next, var);
		}
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
		tok = global_variable(tok, basety, attr);
	}
	return globals;
}
