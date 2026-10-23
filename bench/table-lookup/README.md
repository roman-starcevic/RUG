# Reproducing this optimization's measurements

Measurements are workload-specific, not universal speedups. The original baseline
was fe40fab82c61a6fdd95fa49162a289e703644f1c. Both sides used GCC 16.2.1,
C++98, `-O2 -g -fno-omit-frame-pointer`, no LTO, on an AMD Ryzen 9 3900X,
Arch Linux kernel 7.2.9-arch1-1. Target CPU affinity was fixed; frequency,
thermals and other-core activity were not locked.

Build clean baseline and candidate checkouts with matching flags (the Makefile
does not track header dependencies or flag changes):

```sh
make -C /path/to/baseline fclean
make -C /path/to/baseline CXX=g++ 'CXXFLAGS=-Wall -Wextra -Werror -std=c++98 -O2 -g -fno-omit-frame-pointer'
make -C /path/to/candidate fclean
make -C /path/to/candidate CXX=g++ 'CXXFLAGS=-Wall -Wextra -Werror -std=c++98 -O2 -g -fno-omit-frame-pointer'
python3 bench/table-lookup/compare.py --baseline /path/to/baseline --candidate /path/to/candidate --out /path/to/results
```

Requires Linux taskset, Python 3 and GCC. No Omniscient installation is required
to reproduce elapsed-time comparisons. The script compiles the same driver
against each library, generates deterministic fixtures in a disposable directory,
and retains commands, timings and statistics under `--out`. The libraries must
already have been built with matching compiler/flags. A quick functional smoke
run can use `--pairs 2 --target-seconds 0.02`; use the defaults for measurements.

There are two warmup processes per variant, then ten balanced-order paired
trials per shape, shuffled with seed 5234. Reported speedups/reductions use the
median paired ratio; the JSON also records MAD and bootstrap intervals (5,000
resamples). `measured-pairs.csv` contains the original ten measured pairs per
shape, excluding calibration and warmups. Timing excludes instrumentation.
Fixtures include 1/32/128-rule reachable chains, 4,096 seeded binary bytes,
64 joined arithmetic expressions and the bundled ticket database.

| Dataset | Chunk bytes | Median reduction in parse time |
|---|---:|---:|
| binary | 1 | 20.6% |
| binary | 7 | 20.9% |
| binary | 4096 | 21.1% |
| calculator | 1 | 36.5% |
| calculator | 7 | 37.0% |
| calculator | 4096 | 37.0% |
| large | 1 | 23.5% |
| large | 7 | 26.1% |
| large | 4096 | 25.9% |
| medium | 1 | 24.1% |
| medium | 7 | 25.7% |
| medium | 4096 | 25.9% |
| ticket | 1 | 33.0% |
| ticket | 7 | 33.0% |
| ticket | 4096 | 33.0% |
| tiny | 1 | 23.3% |
| tiny | 7 | 24.3% |
| tiny | 4096 | 23.6% |

The driver measures parse/reset/push/finish separately from grammar construction
and session allocation. It tests chunks of 1, 7 and 4096 bytes. All measured
inputs must be accepted. Paired image sizes were unchanged.

A local Omniscient CPU-time profile identified lookup and accessor costs before
the source change. Five separate fixed-work PMU pairs (10,000 binary parses,
4096-byte chunks each) showed 26.5% fewer median retired instructions. The raw
user-mode counts are in `measured-pmu.csv`; no multiplexing or lost reads was
reported in those retained observations. The initial exploratory multiplexed
counter run was excluded. Sampled profiles are CPU-clock (`pmu:sw`), not hardware
cycle profiles; they were not used as the A/B timing results.

These observations exercised a local `omn 0.1.0` build. Composed USE/latency
analysis was unavailable without additional BPF/tracefs permissions; no such
privilege configuration was changed. Collector CPU cost does not fully measure
in-target instrumentation perturbation. No universal speedup, automatic
optimization or gain with LTO is claimed. The two optimization PRs were measured
independently; their speedups must not be multiplied together.
