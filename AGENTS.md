## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

When the user types `/graphify`, use the installed graphify skill or instructions before doing anything else.

Rules:
- When graphify-out/graph.json exists, run `graphify query "<question>"` before your first grep, search or source-file read for a codebase question, and put this rule in every subagent prompt that explores code. Read raw files only for lines the graph or an earlier read already located. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- Dirty graphify-out/ files are expected after hooks or incremental updates; dirty graph files are not a reason to skip graphify. Only skip graphify if the task is about stale or incorrect graph output, or the user explicitly says not to use it.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
- A committed graphify-out/graph.json is rewritten whole on every build, so a large diff to it in a pull request is expected. Do not flag it as churn or suggest skipping the graph rebuild or its hooks.


# Azula — Project Rules for AI Agents

## Project
Azula is a modern C23 reimagining of the legacy ettercap codebase (network
analysis / MITM tool lineage). Current milestone: modernize the legacy
ettercap header files located in `core/`.

## Hard rules (non-negotiable)
1. Language: ISO C23 only. Use `constexpr`, `auto`, `typeof`, `nullptr`,
   keyword `static_assert`, `enum : type`, `[[nodiscard]]`, `[[noreturn]]`,
   `[[maybe_unused]]`, digit separators (`1'000'000`), `<stdatomic.h>`,
   `<stdint.h>` fixed-width types, `__has_c_attribute` / `__has_attribute` /
   `__has_include` feature detection.
2. Naming: project prefix everywhere.
   - Files: `ec_foo.h` -> `azula_foo.h`
   - Macros / enum constants: `EC_FOO` -> `AZ_FOO`
   - Functions / types / variables: `ec_foo` -> `az_foo`
   - Header guards: `ETTERCAP_FOO_H` -> `AZULA_FOO_H`
   - Update ALL `#include` cross-references between renamed headers.
3. Comments: `//` line comments ONLY. Never `/* */`. Comments in English,
   explain WHY, not WHAT.
4. No hidden control flow: macros must not conceal `return` / `exit` / `goto`
   (the `AZ_TRY` propagation idiom is the single allowed exception).
5. Remove legacy autotools-isms: no `config.h`, no `OS_WINDOWS` / `OS_BSD`
   defines — use compiler built-ins (`_WIN32`, `__APPLE__`, `__linux__`).
6. Do not invent implementations: if a header declares a function whose body
   lived in a missing .c file, keep the declaration, modernize the signature
   (const-correctness, `[[nodiscard]]`, fixed-width types), and mark it
   `// TODO: implementation pending`.
7. Do not create files outside `core/`. Do not touch `src/`, `engine/`,
   `build/`, `target/`.

## Design patterns & principles to apply (name them in comments where used)
- Strategy (function-pointer tables, e.g. pluggable allocator, log sink)
- Dependency Injection (pass dependencies as parameters, no global state)
- RAII via `__attribute__((cleanup))` behind a portable guard
- Parameter Object (group scattered parameters into structs)
- Null Object (safe defaults instead of NULL checks)
- Table-driven dispatch instead of switch/if-else chains
- Intrusive containers via `container_of` (type-checked variant)
- Principles: SRP, DRY, KISS, OCP, fail-fast (`static_assert`,
  `[[noreturn]]`), const-correctness, type safety, graceful degradation
  (feature detection with portable fallbacks).

## Known legacy defects to fix on sight
- `_GNU_SOURCE` / `__USE_GNU` defined after libc headers (must be first or in
  build system).
- Pointer-to-offset casts through `int` (truncation/UB on LP64) — use
  `ptrdiff_t`.
- Multi-evaluation / unparenthesized macro arguments (`MIN`, `MAX`,
  `SEC2NANO`) — replace with inline functions or single-eval macros.
- Aborts hidden inside allocation macros — return status instead.
- Non-const `char *` for string literals.
- `## __VA_ARGS__` GNU extension — use C23 `__VA_OPT__(,)` or make the
  variadic pack contain the format string.
- Exit codes of the process mixed into library status enums — separate them.

## Verification (agent MUST run before finishing)
- Every header must be self-contained and compile standalone:
  `gcc -std=c23 -Wall -Wextra -Wpedantic -Werror -fsyntax-only core/azula_<name>.h`
- No remaining identifiers with `ec_` / `EC_` prefixes:
  `grep -rn '\bEC_[A-Z]\|\bec_[a-z]' core/ || echo CLEAN`