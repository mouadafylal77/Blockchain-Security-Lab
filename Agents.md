# AGENTS.md — Blockchain Security Lab

## Project
Academic blockchain cybersecurity simulation platform (Atelier 2).

## Current objective
Build a complete, tested, reproducible C++17 simulation engine before implementing the backend or frontend.

## Required context
- `docs/ATELIER2_REQUIREMENTS.md`: official assignment; source of truth for requirements.
- `atelier1_ex4.cpp`: original Atelier 1 blockchain implementation; preserve unchanged.
- `docs/ARCHITECTURE.md`: technical design.
- `docs/SIMULATION_MODEL.md`: scientific assumptions and simulation rules.
- `docs/DEVELOPMENT_LOG.md`: progress and remaining work.

Read only the documentation relevant to the current task, but always respect the official requirements.

## Technology
- Engine: C++17, CMake, OpenSSL.
- Testing: automated C++ tests.
- Results: JSON/CSV.
- Future backend: FastAPI.
- Future frontend: HTML/CSS/JavaScript, Retro Web 1998 style.

Do not implement backend/frontend during the engine phase.

## Engineering rules
- Use modular `.h`/`.cpp` files.
- Keep `main.cpp` minimal.
- Prefer a deterministic discrete-event network simulator.
- Separate blockchain, networking, attacks, detection, mitigation and experiments.
- Preserve correctness, scientific reproducibility and maintainability.
- Reuse Atelier 1 code where appropriate.
- Do not invent results, features or scientific claims.

## Workflow
For each task:
1. Inspect relevant code and documentation.
2. Implement one well-defined component.
3. Add or update tests.
4. Run the tests and report the outcome.
5. Update relevant documentation and `DEVELOPMENT_LOG.md`.
6. Report limitations and remaining work.

## Future integration
Design a stable command-line interface and documented JSON/CSV outputs so FastAPI can later invoke the engine without rewriting simulation logic.

## Important
The engine must be independently executable and testable. Do not claim a module is complete until its tests pass.
