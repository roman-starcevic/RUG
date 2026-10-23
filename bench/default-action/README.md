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
python3 bench/default-action/compare.py --baseline /path/to/baseline --candidate /path/to/candidate --out /path/to/results
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

| Dataset | Median baseline ms/build | Median candidate ms/build | Paired median speedup |
|---|---:|---:|---:|
| binary | 61.323 | 2.535 | 24.13x |
| calculator | 21.855 | 1.372 | 15.87x |
| large | 61.787 | 2.242 | 27.57x |
| medium | 15.726 | 0.506 | 31.05x |
| ticket | 31.989 | 1.390 | 22.92x |
| tiny | 0.944 | 0.033 | 28.53x |

The driver measures full repeated Rug construction/build/destruction. The windows
are warm, not cold-start disk tests. Default-action selection changed from a
quadratic scan to a frequency map plus ordered tie selection. Six complete
runtime images compared byte-for-byte equal in the validation campaign.

A local Omniscient profile of 100 large-grammar builds concentrated almost all
samples in layout, emission and TableIR::action. Five separate fixed-work PMU
pairs (100 large-grammar builds each) showed 96.8% fewer median retired instructions.
The user-mode counts are in `measured-pmu.csv`; no multiplexing or lost reads was
reported. The histogram introduces temporary map allocations, at most 257 keys
per row; this is not a memory reduction claim. Final packed images are unchanged.

These observations exercised a local `omn 0.1.0` build. Composed USE/latency
analysis was unavailable without additional BPF/tracefs permissions; no such
privilege configuration was changed. Collector CPU cost does not fully measure
in-target instrumentation perturbation. No universal speedup, automatic
optimization or gain with LTO is claimed. The two optimization PRs were measured
independently; their speedups must not be multiplied together.
