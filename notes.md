# Requirements
- Input: LLVM IR
- Output: LLVM IR annotated with potential explicit and/or implicit information flows
- Required analysis: track tainted values through assignments, arithmetic, loads/stores, branches, and function calls
- Bonus: user-supplied security policy file defining security model, levels, categories, and objects along with their level and category set to give more detailed output based on the security policy
    - Security policy config file might look something like [[config.toml]]

# TODO
- Fix printing