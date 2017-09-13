# Unnamed Language Project

A work-in-progress compiler for an as-yet unnamed language, this project was
begun as an opportunity to develop skills working at lower abstraction levels.

As such, this is intended to focus on being a high-performance and low overhead
language, giving experienced developers a different (hopefully better) tool for
building tightly optimized software.

The initial compiler is being written in C for both performance and to gain
additional domain experience.

# Language Aspirations / Goals

* **Competitive with C.**
  * C has long been the gold standard for low-level and systems programming, as
    a "portable assembly" language.  This compiler should generate executables
    of comparable speed and size, with greater speed, and without introducing a
    transitive dependency on C's standard library.
* **Relocatable code.**
  * In particular, the language should using context-specific representations
    for code.  This allows code to move more easily to wherever it's needed,
    with fewer manual changes required.
  * For example:
    * Function definitions are not restricted to the top-level scope.
    * Declarations (including those with initialization and/or without explicit
      types) should use the same syntax everywhere, and should be equally valid
      everywhere they're accepted.
* **Unrestricted compile-time execution.**
  * This enables a more flexible build process, and enables more powerful tools
    than (e.g.) a pre-processor toolchain.
  * This also includes access to the compiler internals themselves, allowing the
    language to kick off further compile jobs, change in-flight ASTs, or perform
    any number of interesting validations on the code as its compiling.
* **Concrete types.**
  * Types describe the size and structure of data, versus capabilities or
    commonalities.  This is to keep the type system grounded and help users
    focus on the data itself, rather than pursue abstraction for its own sake.
* **Predictable behavior.**
  * When the code indicates that a procedure call should be inlined, it will be
    inlined – or it will fail to compile.  Data structures should have knowable
    sizes before compile time.  Nothing the compiler does should seem "magical",
    unless there's a non-trivial benefit to doing otherwise.
  * This also implies that the language itself should be as simple as possible,
    so that users carry less mental baggage to understand the compiler's
    behavior.
* **Minimally implicit.**
  * This follows along with predictability - implicit behavior adds cognitive
    load and masks complexity.  The compiler should limit implicit behavior to
    things that are obvious, correct, and without consequence.
* **Fewer keywords.**
  * Keywords (and reserved words) obstruct the user's ability to express their
    own program's logic clearly.
* **Good compiler errors.**
  * Syntactic and logical errors raised by the compiler should legitimately be
    *helpful*.  A clear description of the error, and highlighted source code
    are the **minimum** requirement; we can often do better.
* **"Safety third."**
  * Provable safety is a hot topic these days, and one that this language
    specifically ignores.  Instead, the focus is put on making code easier to
    write and change, and to make that process less onerous and more enjoyable.
  * This should not be considered a condemnation of "safe" languages – they are
    absolutely invaluable for many problem domains, and there's significant
    benefits to using them.  The drawbacks, however – linguistic overhead,
    conceptual confusion, code calcification, etc. – add formality to the code
    that make it difficult to "explore" for a solution.
  * If the language is well-designed, "unsafe" code should be clearly visible
    when the user is ready to "tighten the screws".  If not, safety validations
    can be added to the build process at that time.  In the worst case, the code
    represents a reasonable sketch of a solution that can be ported to a "safe"
    language.
* **Enjoyable code.**
  * The above goals all ultimately serve one purpose – to make the language more
    pleasant to read, write, and maintain.  The language should strive to
    make low-level, high-performance code *fun*, and enable abstractions over
    those primitives that don't sacrifice predictability for convenience.

# Syntax Goals

``` cs
// C-style single-line comments
// No block comments

// Built-in types:
// bool                   -- Boolean value
// u8, u16, u32, u64      -- Unsigned integer value
// s8, s16, s32, s64      -- Signed integer value (two's complement)
// real16, real32, real64 -- Floating point value
// byte                   -- Alias for u8
// int                    -- Alias for s64
// string                 -- A "rich pointer" (size, then data) to bytes

// Variable declaration and initialization
v : int
v : int = 32
v := 32  // Type-inferred declaration

// Numeric Literals
decimal := 1_000_000
hex     := 0x10_00_00_00
binary  := 0b1000_0000
float   := 1_000.50

// String Literals
str := "Standard\tStrings"
char := @char(".")  // The '@' sign indicates a compiler directive.

// Procedures
proc := () => {
  // Nested procedures
  p1 := () => {}

  // Procedure overloading
  p2 := (a : u8) => bool { return a % 2 }
  p2 := (a : s8) => bool { return a % 2 }

  // Multiple returns
  p3 := (a : int) => (int, int) { return (a / 2, a % 2) }

  // Named returns
  p4 := () => (success : bool, result : int) {
    success = false
    return
  }

  // Default parameters
  p5 := (a := "Hello", b := (x : int) => int { return x }) => { }
}

// Top-level declarations are order independent.
a := () => { c() }
b := () => { }
c := () => { b() }

// Iteration
loop {
  break
}

// The `@load` directive
@load("other")  // Loads "other.xxx" into the global namespace.
@load("other")  // Duplicated loads are properly handled.

// "Namespaces" can be created by using the load directive as an expression.
ns := @load("other")

// Similarly, selective imports and aliasing can be done by extracting
// particular members from the load expression.
fn := @load("other").fn
popular := @load("other").unpopular_name

// User-defined operators
@operator(*•_•*) := (a : int, b : int) => int {
  // ...
}

// "Modifying" directives
// These directives act as annotations for the compiler, altering how the
// generated code behaves.
fn := @inline (x : int) => int { return x }

// User-defined "tag" annotations
// These annotations have no compiler-specific meaning, but act as introspection
// hooks for build-time validators or "macro" facilities.
temp := #temp allocate_lots_of_memory()

// Units
// If types describe the *structure* of the data, then units describe *intent*.
t : int<ms> = 15ms
l : string<line> = "This is a line of text."
w : string<word> = "This"

// This feature is only partially decided, and may take on one of a few
// implementations:
//
//   * Level 0 - Units are strictly a documentation feature, and are completely
//     ignored by the compiler after parsing.
//   * Level 1 - Units are considered during typechecking, but silently coerce
//     between unit-specified and unit-free values.
//   * Level 1.5 - Unit aliases are available, so that `int<s>`, `int<sec>`, and
//     `int<seconds>` are all understood to have the same unit type.
//   * Level 2 - Unit conversion functions may be specified, allowing for
//     implicit transformation between unit types.
//   * Level n - Full dimensional analysis is built-in, allowing expressions
//     like `15ft / 3min / 2hr` to satisfy an argument of `int<m/s^2>`.
```
