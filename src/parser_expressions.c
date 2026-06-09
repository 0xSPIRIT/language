#include "parser.h"

typedef enum {
    PREC_NONE = 0,
    PREC_ASSIGN,    // = += -= *= /= %=
    PREC_OR,        // ||
    PREC_AND,       // &&
    PREC_EQUALITY,  // == !=
    PREC_COMPARE,   // < > <= >=
    PREC_TERM,      // + -
    PREC_FACTOR,    // * / %
    PREC_UNARY,     // - ! (prefix) ++x
    PREC_POSTFIX,   // x++ x--
    PREC_CALL,      // f(...)
} precedence;

static precedence infix_precedence(parser *p) {
    switch (peek(p)->Type) {
        case TOKEN_OR:            return PREC_OR;
        case TOKEN_AND:           return PREC_AND;
        case TOKEN_EQUALS_EQUALS: return PREC_EQUALITY;
        case TOKEN_BANG_EQUALS:   return PREC_EQUALITY;
        case TOKEN_LESS:          return PREC_COMPARE;
        case TOKEN_MORE:          return PREC_COMPARE;
        case TOKEN_LESS_EQUALS:   return PREC_COMPARE;
        case TOKEN_MORE_EQUALS:   return PREC_COMPARE;
        case TOKEN_PLUS:          return PREC_TERM;
        case TOKEN_MINUS:         return PREC_TERM;
        case TOKEN_MULTIPLY:      return PREC_FACTOR;
        case TOKEN_DIVIDE:        return PREC_FACTOR;
        case TOKEN_PERCENT:       return PREC_FACTOR;

        // Compound assignment — right-associative, handled specially below.
        case TOKEN_EQUALS:
        case TOKEN_PLUS_EQ:
        case TOKEN_MINUS_EQ:
        case TOKEN_TIMES_EQ:
        case TOKEN_DIV_EQ:
        case TOKEN_MOD_EQ:   return PREC_ASSIGN;

        // Postfix ++ is an infix-position operator with no right operand.
        case TOKEN_POST_INC: return PREC_POSTFIX;

        default: return PREC_NONE;
    }
}

// Maps a compound-assignment token to the underlying arithmetic operator,
// e.g.  TOKEN_PLUS_EQ  →  TOKEN_PLUS  so  x += y  becomes  x = x + y.
static token_type compound_assign_op(token_type t) {
    switch (t) {
        case TOKEN_PLUS_EQ:  return TOKEN_PLUS;
        case TOKEN_MINUS_EQ: return TOKEN_MINUS;
        case TOKEN_TIMES_EQ: return TOKEN_MULTIPLY;
        case TOKEN_DIV_EQ:   return TOKEN_DIVIDE;
        case TOKEN_MOD_EQ:   return TOKEN_PERCENT;
        default:             return TOKEN_NONE;
    }
}

static ast_node *parse_expression_prec(parser *p, precedence min_prec);

static ast_node *parse_prefix(parser *p) {
    token *Tok = peek(p);

    // Prefix ++x  →  NODE_UNARY_OP with Operation = TOKEN_PRE_INC
    if (Tok->Type == TOKEN_PRE_INC) {
        advance(p);
        ast_node *Operand = parse_expression_prec(p, PREC_UNARY);
        if (!Operand) {
            parse_error(p, "Expected expression after prefix '++'.");
            return NULL;
        }
        ast_node *N          = node(p, NODE_UNARY_OP);
        N->UnaryOp.Operation = TOKEN_PRE_INC;
        N->UnaryOp.Operand   = Operand;
        return N;
    }

    // Unary: -expr  !expr
    if (Tok->Type == TOKEN_MINUS || Tok->Type == TOKEN_BANG) {
        advance(p);
        ast_node *Operand = parse_expression_prec(p, PREC_UNARY);
        if (!Operand) {
            parse_error(p, "Expected expression after unary operator.");
            return NULL;
        }
        ast_node *N          = node(p, NODE_UNARY_OP);
        N->UnaryOp.Operation = Tok->Type;
        N->UnaryOp.Operand   = Operand;
        return N;
    }

    // Grouped: ( expr )
    if (Tok->Type == TOKEN_OPEN_PAREN) {
        advance(p);
        ast_node *Inner = parse_expression_prec(p, PREC_NONE);
        consume(p, TOKEN_CLOSE_PAREN);
        return Inner;
    }

    // Function call: identifier followed by '('
    if (is_function_call(p)) {
        return parse_function_call(p);
    }

    // Plain identifier
    if (Tok->Type == TOKEN_IDENTIFIER) {
        return parse_identifier(p);
    }

    // Literals: numbers, strings, chars
    if (Tok->Type == TOKEN_NUMBER || Tok->Type == TOKEN_QUOTE || Tok->Type == TOKEN_CHAR_QUOTE) {
        return parse_literal(p);
    }

    parse_error(p, "Unexpected token '%s' in expression.", token_name(Tok->Type));
    return NULL;
}

static ast_node *parse_expression_prec(parser *p, precedence min_prec) {
    ast_node *Left = parse_prefix(p);
    if (!Left) return NULL;

    while (true) {
        precedence prec = infix_precedence(p);
        if (prec <= min_prec) break;

        token *Op = advance(p);

        // Postfix ++  →  NODE_UNARY_OP, no right-hand side to parse.
        if (Op->Type == TOKEN_POST_INC) {
            ast_node *N          = node(p, NODE_UNARY_OP);
            N->UnaryOp.Operation = TOKEN_POST_INC;
            N->UnaryOp.Operand   = Left;
            Left                 = N;
            continue;
        }

        // Plain assignment  x = expr  — right-associative, so recurse at prec-1.
        if (Op->Type == TOKEN_EQUALS) {
            ast_node *Right = parse_expression_prec(p, PREC_ASSIGN - 1);
            if (!Right) {
                parse_error(p, "Expected right-hand side after '='.");
                return NULL;
            }
            ast_node *N = node(p, NODE_ASSIGN);
            // Reuse the target name from Left if it's a plain identifier;
            // the caller's is_var_assign guard means Left should always be one.
            N->Assign.Target = Left->Ident.Name;
            N->Assign.Value  = Right;
            Left             = N;
            continue;
        }

        // Compound assignment  x += expr  →  x = x + expr, right-associative.
        token_type ArithOp = compound_assign_op(Op->Type);
        if (ArithOp != TOKEN_NONE) {
            ast_node *Right = parse_expression_prec(p, PREC_ASSIGN - 1);
            if (!Right) {
                parse_error(p, "Expected right-hand side after '%s'.", token_name(Op->Type));
                return NULL;
            }
            // Build the inner  x + expr  node.
            ast_node *BinOp           = node(p, NODE_BINARY_OP);
            BinOp->BinaryOp.Operation = ArithOp;
            BinOp->BinaryOp.Left      = Left;
            BinOp->BinaryOp.Right     = Right;

            // Wrap in an assignment.
            ast_node *N      = node(p, NODE_ASSIGN);
            N->Assign.Target = Left->Ident.Name;
            N->Assign.Value  = BinOp;
            Left             = N;
            continue;
        }

        // All remaining operators are left-associative binary ops.
        // Passing `prec` (not prec-1) as min_prec gives left-associativity:
        // 1 - 2 - 3  parses as  (1 - 2) - 3.
        ast_node *Right = parse_expression_prec(p, prec);
        if (!Right) {
            parse_error(p, "Expected right-hand side after '%s'.", token_name(Op->Type));
            return NULL;
        }

        ast_node *BinOp           = node(p, NODE_BINARY_OP);
        BinOp->BinaryOp.Operation = Op->Type;
        BinOp->BinaryOp.Left      = Left;
        BinOp->BinaryOp.Right     = Right;
        Left                      = BinOp;
    }

    return Left;
}

ast_node *parse_expression(parser *p) { return parse_expression_prec(p, PREC_NONE); }
