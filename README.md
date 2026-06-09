# LLVM Information Flow Analyzer

This project parses LLVM IR into control-flow graphs and runs information-flow
analysis against a policy supplied in a TOML-style config file.

The current analyzer supports Bell-LaPadula confidentiality checks and Biba
integrity checks. It reports flows that violate the configured policy.

## Build

```bash
make
```

Run the analysis tests:

```bash
make test
```

## LLVM Input

Compile C programs to unoptimized LLVM IR:

```bash
clang -S -emit-llvm -O0 file.c -o file.ll
```

Optimized IR from `-O1`, `-O2`, or `-O3` is not guaranteed to parse or analyze
correctly.

## Usage

Analyze with the default policy:

```bash
./analyzer input.ll
```

Analyze with a config file:

```bash
./analyzer --config BLP_config.toml input.ll
```

Print CFGs:

```bash
./analyzer --print-cfg input.ll
```

Print the parsed policy and restriction summary:

```bash
./analyzer --config BLP_config.toml --print-policy
```

## Policy Config

Configs define:

- enabled model names
- confidentiality and/or integrity levels
- categories
- policy objects
- effects for external or policy-boundary functions

Example:

```toml
[model]
names = ["Bell-LaPadula"]

[levels]
order = ["Public", "Secret"]

[objects]
"public_output" = { level = "Public", categories = [] }
"secret_input" = { level = "Secret", categories = [] }

[effects.SOURCE]
reads = "secret_input"
returns = true

[effects.SINK]
writes = "public_output"
reads_from_args = [0]
```

\*More examples are provided, called *_config.toml

For Bell-LaPadula, a flow from source object `A` to destination object `B` is
allowed only when `B` dominates `A`.

For Biba, a flow from source object `A` to destination object `B` is allowed
only when `A` dominates `B` in integrity.

## Effects

Effects connect LLVM calls to policy objects.

Supported effect fields:

- `reads = "object"`: function reads from a policy object
- `writes = "object"`: function writes to a policy object
- `returns = true`: function return value receives the read object's label
- `reads_from_args = [0, 1]`: written object receives data from these arguments
- `writes_to_args = [1]`: pointer arguments receive data from the read object

Examples:

```toml
[effects.scanf]
reads = "stdin"
writes_to_args = [1]

[effects.printf]
writes = "stdout"
reads_from_args = [1, 2]
```

Config effects should describe policy boundaries such as files, terminal I/O,
network I/O, database reads, and audit writes. Internal helper functions do not
usually need config effects because the analyzer infers function summaries from
their LLVM bodies.

## Analysis Support

The analyzer tracks:

- explicit flows through values and simple memory locations
- implicit flows through branch and switch conditions
- scoped program-counter labels that end at branch reconvergence points
- function summaries for internal helper functions
- source/sink behavior from configured effects
- Bell-LaPadula and Biba violations

Internal function summaries infer:

- which arguments influence a return value
- which arguments influence pointer writes
- when return or pointer writes receive labels from internal calls to configured
  source effects

## Supported LLVM IR Subset

The parser targets ordinary unoptimized C lowered to readable LLVM IR.

Supported instructions and constructs:

- function definitions and parameters
- basic blocks and labels
- `alloca`
- `load`
- `store`
- `getelementptr`
- `ret`
- `br`
- `switch`
- `phi`
- `call`
- integer arithmetic: `add`, `sub`, `mul`, `sdiv`, `udiv`
- remainder and bitwise ops: `srem`, `urem`, `and`, `or`, `xor`, `shl`, `lshr`, `ashr`
- floating arithmetic: `fadd`, `fsub`, `fmul`, `fdiv`, `frem`
- comparisons: `icmp`, `fcmp`
- casts/conversions: `zext`, `sext`, `trunc`, `bitcast`, `ptrtoint`, `inttoptr`,
  `fptrunc`, `fpext`, `fptoui`, `fptosi`, `uitofp`, `sitofp`
- `select`
- direct function calls, including simple varargs

Unsupported or out of scope:

- exception handling: `invoke`, `landingpad`, `resume`
- atomics: `cmpxchg`, `atomicrmw`, atomic load/store semantics
- vector operations
- inline assembly
- coroutines
- `indirectbr`, `callbr`
- full struct/aggregate semantics beyond simple pointer/GEP parsing
- full debug metadata interpretation

Unsupported instructions are reported as:

```text
Unsupported LLVM instruction: ...
```

## Examples

### Fibonacci

`examples/fib` demonstrates a normal public-input/public-output program.

```bash
cd examples/fib
make CC=/usr/bin/clang
../../analyzer --config fib_config.toml fib.ll
```

Expected result:

```text
INFO FLOW: no violations found
```

### End-To-End Bank Flow

`examples/end_to_end` demonstrates:

- allowed public-to-public flow
- explicit secret-to-public leak
- implicit secret-to-public leak
- internal helper function summaries
- allowed secret-to-secret flow

```bash
cd examples/end_to_end
make CC=/usr/bin/clang
make analyze
```

Expected result: the analyzer reports Bell-LaPadula violations for the explicit
and implicit public leaks, but allows the public flow and secret audit flow.

## Known Limitations

- The parser is not a full LLVM frontend.
- Analysis is conservative and may over-approximate some flows.
- Alias analysis is simple: pointer names are treated as simple memory locations.
- Function summaries are designed for straightforward helper functions.
- Configured effects still define the semantic meaning of external policy
  boundaries.
