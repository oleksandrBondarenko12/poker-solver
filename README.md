# PokerSolver 🎲🃏

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/CMake-3.10+-brightgreen.svg)](https://cmake.org/)
[![Qt6](https://img.shields.io/badge/Qt-6.x-green.svg)](https://www.qt.io/)
[![OpenMP](https://img.shields.io/badge/OpenMP-Enabled-orange.svg)](https://www.openmp.org/)
[![Tests](https://img.shields.io/badge/Tests-105%20Passing-success.svg)](tests/)

**PokerSolver** is a high-performance, mathematically rigorous C++20 Counterfactual Regret Minimization (CFR+) equilibrium solver and interactive desktop analysis suite for Heads-Up Texas Hold'em. 

It pairs an optimized CFR+ solving engine—featuring 64-bit bitboard card algebra, canonical flop isomorphism reduction, and zero-sum EV invariance—with a modern Qt6 graphical user interface and GTO Strategy Explorer.

---

## Current Design Status 🎨

Below are screenshots of the interactive desktop application:

### Main Solver Configuration Window
![Main Window Design](images/screenshots/main-window.png)
*Configures initial board cards, pot size, effective stacks, and granular per-street betting geometry.*

### Range Selection Matrix
![Range Selector Design](images/screenshots/range-selector.png)
*Visual 13×13 matrix dialog for configuring IP and OOP preflop hand ranges with custom weights.*

### GTO Strategy Explorer
![Strategy Explorer Design](images/screenshots/strategy-explorer.png)
*Real-time strategy inspection window featuring action mix bars, EV heatmaps, combo breakdowns, and blocker detection.*

---

## Key Features 🌟

- **C++20 CFR+ Engine:** Implements Discounted CFR / CFR+ with regret matching+, linear strategy weighting, and reach-probability normalization.
- **Strict Mathematical Invariants:** Guaranteed zero-sum EV conservation ($EV_{IP} + EV_{OOP} = 0$) and backward-induction exploitability calculation via the built-in [BestResponseCalculator](include/poker_solver/solver/exploitability_calculator.h).
- **Canonical Flop Isomorphism:** Exploits suit isomorphism to collapse all 22,100 Hold'em flops into 1,755 canonical abstractions, drastically saving memory and computation.
- **Fast 5-Card & 7-Card Hand Evaluation:** High-speed lookup table evaluator with binary caching (`five_card_strength.bin`) and bitwise board overlap checks.
- **Extensive-Form Game Tree Builder:** Full street progression (Preflop $\rightarrow$ Flop $\rightarrow$ Turn $\rightarrow$ River) supporting customizable bet sizes, raises, and all-ins.
- **Multi-Threaded Acceleration:** Parallelized across public chance dealing using OpenMP.
- **Interactive Qt6 GUI & GTO Explorer:**
  - Dynamic 13×13 hand grid with proportional action frequencies and EV heatmaps.
  - Card blocker awareness (dead cards dynamically crossed out in combo views).
  - Virtualized on-demand game tree hierarchy navigation.
  - Asynchronous background solver execution with zero-copy JSON IPC.

---

## Project Structure 📁

```
poker-solver/
├── CMakeLists.txt          # CMake build configuration (C++20, Qt6, OpenMP, ASan)
├── README.md               # Project overview and quick start guide
├── CODEBASE_GUIDE.md       # Comprehensive architectural deep dive for all files
├── include/                # Public headers organized strictly by subsystem
│   └── poker_solver/
│       ├── evaluator/      # Hand strength evaluators & lookup table dictionary
│       ├── core/           # 64-bit bitboard cards, deck, math, flop isomorphism
│       ├── kuhn/           # Kuhn poker benchmark setup
│       ├── ranges/         # Range syntax parser, combos, river evaluation cache
│       ├── solver/         # CFR+ engines, exploitability & best response calculator
│       └── tree/           # Extensive game tree nodes, actions, and scenario rules
├── src/                    # Source implementations matching include/
│   ├── core/
│   ├── evaluator/
│   ├── gui/                # Qt6 desktop application, explorer & dark styling
│   ├── kuhn/
│   ├── ranges/
│   ├── solver/
│   └── tree/
├── tests/                  # GoogleTest unit, integration, and exploitability suites
├── test_data/              # Benchmark scenario configs and hand rank dictionaries
├── third_party/            # External dependencies (googletest, nlohmann/json)
│   ├── googletest/
│   └── nlohmann/
├── docs/                   # Self-study and step-by-step verification guide
│   └── VERIFICATION_GUIDE.md
├── mockups/                # UI HTML and TSX design references
├── scripts/                # Verification scripts (e.g., canonical flop isomorphism)
└── images/
    └── screenshots/        # Application UI screenshots
```

> - For a complete file-by-file breakdown of algorithms, data structures, and subsystems, read the [CODEBASE_GUIDE.md](CODEBASE_GUIDE.md).
> - For a step-by-step bottom-up verification and self-study plan, read the [docs/VERIFICATION_GUIDE.md](docs/VERIFICATION_GUIDE.md).

---

## Installation & Build 🔧

### Prerequisites
- **Compiler:** Modern C++20 compliant compiler (Apple Clang 14+, GCC 11+, or Clang 13+)
- **Build System:** CMake $\ge 3.10$
- **GUI Toolkit:** Qt6 (Widgets, Gui, Core)
- **Parallelism:** OpenMP runtime (`libomp` on macOS via Homebrew)

### Building on macOS / Linux

1. **Clone the repository:**
   ```bash
   git clone --recursive https://github.com/oleksandrBondarenko12/poker-solver.git
   cd poker-solver
   ```

2. **Configure with CMake:**
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```

3. **Compile all targets:**
   ```bash
   cmake --build build -j
   ```

---

## Executables & Verification 🎮

### 1. Main Graphical Application (`poker_solver_ui`)
Launch the full desktop solver configuration and exploration app:
```bash
./build/poker_solver_ui
```

### 2. Standalone Strategy Explorer (`test_explorer`)
Inspect and interact with any dumped solver JSON strategy:
```bash
# Launch interactive explorer window
./build/test_explorer test_data/custom_gui_scenario.json

# Headless automated validation
./build/test_explorer --verify test_data/custom_gui_scenario.json
```

### 3. Comprehensive Test Suite (`poker_solver_tests`)
Run the 107 automated tests spanning bitboards, isomorphism, CFR convergence, and exploitability bounds:
```bash
./build/poker_solver_tests
```

Or run targeted fast unit tests (105 tests execute in ~2.2s):
```bash
./build/poker_solver_tests --gtest_filter="-*CustomGui*:*GenerateGuiOutput*"
```

---

## Verification & Exploitability Benchmarks 📊

The solver is verified against theoretical zero-sum equilibrium benchmarks:

| Benchmark Scenario | Board | Pot | Convergence / Exploitability | Zero-Sum Invariance |
| :--- | :--- | :--- | :--- | :--- |
| **`SimpleRiverTest`** | `Ts Jh 2h 4c 5d` | 30.0 | **0.00017%** ($1.7 \times 10^{-4}\%$) | $EV_{IP} + EV_{OOP} = 0.00000000$ |
| **`SimpleFlopTest`** | `Ts Jh 2h` | 30.0 | **0.1747%** | $EV_{IP} + EV_{OOP} = -0.0000$ |
| **`CustomGuiScenarioTest`** | `Qs Jh 2h` | 50.0 | **0.3342%** (after 200 iterations) | $EV_{IP} + EV_{OOP} = 0.00000000$ |
| **Injected Vulnerability Test** | `Ts Jh 2h 4c 5d` | 30.0 | Jumps to **25% - 44%** on leaks | Proves exploitability sensitivity |

---

## Dependencies 📦

- **[Qt6](https://www.qt.io/)**: Widgets, Gui, Core for native cross-platform GUI rendering.
- **[OpenMP](https://www.openmp.org/)**: Multi-threaded game tree chance node sweep.
- **[nlohmann/json](https://github.com/nlohmann/json)**: Modern C++ JSON library bundled in `third_party/nlohmann/`.
- **[Google Test](https://github.com/google/googletest)**: C++ testing framework bundled in `third_party/googletest/`.

---

## Contributing 🤝

Contributions are welcome! Please follow these guidelines:
1. Ensure all code conforms to modern C++20 and snake_case naming conventions.
2. Maintain zero loose files under `include/poker_solver/` by organizing headers into appropriate subsystems.
3. Verify that all tests pass (`./build/poker_solver_tests`) with zero exploitability regressions before submitting pull requests.

---

## License 📄

This project is licensed under the MIT License.