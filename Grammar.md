# Grammar (WIP)

    NAMESPACE = ((DECLARATION | NS_DIRECTIVE) "\n")*
    NS_DIRECTIVE = "@import" String
                 | "@operator" OP_DECLARATION
                 | "@run" EXPRESSION
    DECLARATION = Ident ":" TYPE
                | Ident ":" TYPE "=" EXPRESSION
                | Ident ":=" EXPRESSION
    OP_DECLARATION = Operator ":" TYPE
                   | Operator ":" TYPE "=" EXPRESSION
                   | Operator ":=" EXPRESSION
    TYPE = Ident
    EXPRESSION = PROCEDURE_EXPR
               | "(" EXPRESSION ")"
               | EXPRESSION Operator EXPRESSION
               | Operator EXPRESSION
               | EXPRESSION "(" EXPRESSION_LIST? ")"
               | Literal
               | Ident
    PROCEDURE_EXPR = "(" ARGUMENT_LIST? ")" "=>" RETURN_TYPES? CODE_BLOCK
    ARGUMENT_LIST = DECLARATION ("," DECLARATION)*
    EXPRESSION_LIST = EXPRESSION ("," EXPRESSION)*
    RETURN_TYPE_LIST = RETURN_TYPE ("," RETURN_TYPE)*
    RETURN_TYPES = TYPE
                 | "(" RETURN_TYPE_LIST? ")"
    RETURN_TYPE = (Ident ":")? TYPE
    CODE_BLOCK = "{" BLOCK_BODY? "}"
    BLOCK_BODY = DECLARATION
               | EXPRESSION

# Questions

* What else do we want to permit in the global context?
  * Probably assignments, when we have some separate notion of that.
* What global directives are we missing?
* What more complex type statements do we want to support?
* Are non-identifiers useful on the LHS of a declaration?

# To Do

* Actually implement NS_DIRECTIVE behaviors.
* Expand TYPE declarations.
