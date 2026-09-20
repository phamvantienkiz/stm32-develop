# AGENTS.md — Project Intelligence File

> This file is the primary control layer for Antigravity CLI (`agy`) and Antigravity 2.0. Read it completely at the start of every session, after any `/clear`, or upon context compression. Follow these rules strictly throughout the entire session.

---

## 1. Project Context

- **Project Name**: STM32F429 Development
- **Current Goal**: Support the analysis and development of projects involving STM32 boards.
- **Infrastructure Context**: MCP servers available via `.agents/mcp_config.json`.

---

## 2. Core Philosophy (Caution Over Speed)

### Think Before Coding

- **Don't assume, don't hide confusion, and always surface tradeoffs.**
- State your assumptions explicitly before implementing any solution; if uncertain, ask the user immediately.
- If multiple interpretations or approaches exist, present them clearly instead of picking one silently.
- If a simpler or more elegant approach exists, push back and suggest it before writing code.
- If something within the requirements is unclear, **STOP** and name what is confusing.

### Simplicity First

- **Write the minimum code that solves the problem. Absolutely nothing speculative.**
- Do not implement any features, abstractions, or "future-proofing" configurability beyond what was explicitly requested.
- Avoid adding complex error handling for impossible or out-of-scope scenarios.
- If a solution can be written in 50 lines instead of 200, rewrite and simplify it ruthlessly.

### Surgical Changes

- **Touch only what you must. Clean up only your own mess.**
- Do not "improve", reformat, or refactor adjacent code or comments that are not broken or requested.
- Strictly match the existing codebase style, naming conventions, and architecture patterns.
- If your changes create orphans (unused imports, variables, or functions), remove them immediately. Do not touch pre-existing dead code unless explicitly instructed.
- Every changed line must trace directly and cleanly back to the user's request.
