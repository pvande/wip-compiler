## AstNode

This struct represents a single node in the AST; all nodes in the tree will be
instances of this single struct.  Consequently, there are a number of fields
which may only be conditionally set.

With each `type` and `flags` combination, some set of relevant fields will be
populated.  Among other things, this document aims to record which fields will
be populated (and when).


    // The AstNode struct layout
    AstNodeType type;
    AstNodeFlags flags;

    FileAddress from;
    FileAddress to;

    Symbol ident;
    String source;
    struct AstNode* lhs;
    List* body;
    struct AstNode* rhs;

    String* error;


So long as the AstNode was created by the `get_node` helper in the parser, the
following truths will hold:

* `type` will be set to a useful value.
* `flags` will have been initialized to zero.
* `from` will be initialized to the line and column being inspected at
  initialization time.
* `error` will be initialized to `NULL`.

Additionally, you may also rely on the following:

* `to` will be populated with the line and column of the last character
  of the final token parsed in generating the node.

The values in all other fields should be ignored unless otherwise stated below.


## Node Types

### NODE_ASSIGNMENT

An assignment node is generated whenever the code assigns a new value to an
identifier.  This is indicated in code with the `=` operator, though the `:=`
operator provides a shorthand for "declare and assign".

The assigned-to `identifier` will be stored in the struct's `ident` field.    @Lazy

The `value` expression will be stored in the structs `rhs` field.

Assignment nodes have no special `flags`.

### NODE_BRANCH

Not yet implemented.

### NODE_COMPOUND

A compound node represents a decomposition of a more complex syntactical
construction.  Which construction it represents – and therefore, where data will
be stored – will be designated by `flags`.

#### COMPOUND_DECL_ASSIGN

Created when parsing a unified declaration and assignment, the `lhs` of this
node contains the declaration itself, and the `rhs` contains a well-formed
assignment.

### NODE_DECLARATION

A declaration node is generated whenever the source code introduces a new
variable.  Declarations have three forms:

* `<identifier> : <type>`
* `<identifier> : <type> = <value>`
* `<identifier> := <value>`  (@TODO)

In all three cases, the `identifier` will be stored in the struct's `ident`
field.

If the declaration contains a `type`, that type expression will be stored in the
structs `rhs`; if no type is specified, the `rhs` slot will be explicitly `NULL` (@TODO).

If a `value` is provided, a separate assignment node will be created.

Declaration nodes have no special `flags`.

### NODE_EXPRESSION

To be documented.

### NODE_LOOP

Not yet implemented.

### NODE_RECOVERY

Recovery nodes are created when parsing an unrecognized phrase or statement.
Whatever part of the statement could be successfully parsed is stored in `lhs`,
and the `from` and `to` fields are set to cover whatever tokens were
unrecognized.

Recovery nodes will always have a relevant `error` field.

### NODE_TYPE

Type nodes are created when parsing a type expression.  At present, the type
identifier is stored in the `ident` field.

Type nodes have no special flags.

### NODE_TUPLE

Not yet implemented.
