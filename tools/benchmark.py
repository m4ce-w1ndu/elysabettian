#!/usr/bin/env python3
"""
Elysabettian Scientific Benchmarking Suite (C++ vs Rust & VM vs JIT)
Performs multi-iteration statistical analysis and visualization comparing:
  - C++ Bytecode VM vs C++ AsmJit Native JIT
  - C++ (AsmJit) vs Rust (Cranelift) Head-to-Head Performance
"""

import os
import sys
import time
import math
import json
import argparse
import subprocess
from statistics import mean, median, stdev
from pathlib import Path

# Paths
ROOT_DIR = Path(__file__).resolve().parent.parent
CPP_RELEASE_BIN = ROOT_DIR / "build" / "release" / "tools" / "ely_cli" / "ely_cli"
CPP_DEFAULT_BIN = ROOT_DIR / "build" / "default" / "tools" / "ely_cli" / "ely_cli"
RUST_ROOT = ROOT_DIR.parent / "elysabettian-rs"
RUST_RELEASE_BIN = RUST_ROOT / "target" / "release" / "ely_cli"

EXAMPLES_DIR = ROOT_DIR / "examples"
RESULTS_DIR = ROOT_DIR / "tools" / "results"

BENCHMARKS = [
    {
        "id": "bench_loop",
        "file": "bench_loop.ely",
        "name": "Accumulator Loop",
        "desc": "Numeric accumulator loop (2,000,000 iters)",
    },
    {
        "id": "bench_arithmetic",
        "file": "bench_arithmetic.ely",
        "name": "Transcendental Math",
        "desc": "Floating-point multiply/divide with Pi & e (2,000,000 iters)",
    },
    {
        "id": "bench_fibonacci",
        "file": "bench_fibonacci.ely",
        "name": "Fibonacci Modulo",
        "desc": "Iterative Fibonacci sequence (1,000,000 iters)",
    },
    {
        "id": "bench_leibniz_pi",
        "file": "bench_leibniz_pi.ely",
        "name": "Leibniz Pi Series",
        "desc": "Alternating infinite series Pi approximation (5,000,000 iters)",
    },
    {
        "id": "bench_collatz",
        "file": "bench_collatz.ely",
        "name": "Collatz Conjecture",
        "desc": "3n + 1 trajectory steps using bitwise shifts (25,000 ranges)",
    },
]

def ensure_cpp_binary():
    if CPP_RELEASE_BIN.exists():
        return CPP_RELEASE_BIN
    print("[*] Compiling C++ ely_cli in release mode (cmake --preset release)...")
    res = subprocess.run(
        ["cmake", "--preset", "release"],
        cwd=ROOT_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if res.returncode != 0:
        print("[!] CMake configure failed:\n", res.stderr)
        if CPP_DEFAULT_BIN.exists():
            print("[*] Falling back to default debug binary...")
            return CPP_DEFAULT_BIN
        sys.exit(1)

    res = subprocess.run(
        ["cmake", "--build", "--preset", "release"],
        cwd=ROOT_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if res.returncode != 0:
        print("[!] CMake build failed:\n", res.stderr)
        if CPP_DEFAULT_BIN.exists():
            return CPP_DEFAULT_BIN
        sys.exit(1)

    print("[+] C++ Release binary ready at:", CPP_RELEASE_BIN, "\n")
    return CPP_RELEASE_BIN

def run_single(cmd):
    start = time.perf_counter()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    elapsed = time.perf_counter() - start
    if proc.returncode != 0:
        raise RuntimeError(f"Command failed (exit {proc.returncode}): {proc.stderr.strip()}")
    return elapsed, proc.stdout.strip()

def benchmark_workload(bin_path, bench_path, mode_flag, warmup_runs=2, measured_runs=5):
    cmd = [str(bin_path), mode_flag, str(bench_path)]

    # Warmup
    last_output = None
    for _ in range(warmup_runs):
        _, last_output = run_single(cmd)

    # Measured runs
    timings = []
    for _ in range(measured_runs):
        elapsed, last_output = run_single(cmd)
        timings.append(elapsed)

    std_val = stdev(timings) if len(timings) > 1 else 0.0
    return {
        "timings": timings,
        "mean": mean(timings),
        "median": median(timings),
        "stdev": std_val,
        "min": min(timings),
        "max": max(timings),
        "output": last_output,
    }

def check_parity(out1, out2):
    if out1 == out2:
        return True
    try:
        f1 = float(out1)
        f2 = float(out2)
        return math.isclose(f1, f2, rel_tol=1e-5, abs_tol=1e-5)
    except (ValueError, TypeError):
        return False

def generate_charts(results, has_rust=False):
    try:
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        print("[!] matplotlib or numpy not installed. Skipping graphical chart generation.")
        return

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    labels = [b["name"] for b in results]
    cpp_vm_means = [b["cpp_vm"]["mean"] for b in results]
    cpp_vm_errs = [b["cpp_vm"]["stdev"] for b in results]
    cpp_jit_means = [b["cpp_jit"]["mean"] for b in results]
    cpp_jit_errs = [b["cpp_jit"]["stdev"] for b in results]
    speedups = [b["cpp_speedup"] for b in results]

    # Chart 1: Absolute Execution Times (C++ VM vs C++ AsmJit)
    x = np.arange(len(labels))
    width = 0.35

    fig, ax = plt.subplots(figsize=(10, 6))
    rects1 = ax.bar(x - width/2, cpp_vm_means, width, yerr=cpp_vm_errs, capsize=4,
                    label='C++ VM Interpreter (--no-jit)', color='#e05d44', alpha=0.9)
    rects2 = ax.bar(x + width/2, cpp_jit_means, width, yerr=cpp_jit_errs, capsize=4,
                    label='C++ AsmJit JIT (--jit)', color='#007ec6', alpha=0.9)

    ax.set_ylabel('Execution Time (seconds) - lower is better', fontsize=12)
    ax.set_title('Elysabettian C++: VM vs AsmJit Native JIT (Mean ± StdDev)', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha='right', fontsize=11)
    ax.legend(fontsize=11)
    ax.grid(axis='y', linestyle='--', alpha=0.5)

    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            ax.annotate(f'{height:.3f}s',
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),
                        textcoords="offset points",
                        ha='center', va='bottom', fontsize=9)
    autolabel(rects1)
    autolabel(rects2)

    fig.tight_layout()
    chart1_path = RESULTS_DIR / "benchmark_execution_times.png"
    plt.savefig(chart1_path, dpi=200)
    plt.close()
    print(f"[+] Execution time chart saved to: {chart1_path}")

    # Chart 2: Speedup Factor (C++ JIT vs C++ VM)
    fig, ax = plt.subplots(figsize=(9, 5))
    y_pos = np.arange(len(labels))
    bars = ax.barh(y_pos, speedups, color='#2ca02c', alpha=0.85, height=0.55)

    ax.set_xlabel('Speedup Factor (VM Time / JIT Time) - higher is better', fontsize=12)
    ax.set_title('AsmJit JIT Acceleration Factor (C++)', fontsize=14, fontweight='bold')
    ax.set_yticks(y_pos)
    ax.set_yticklabels(labels, fontsize=11)
    ax.axvline(1.0, color='red', linestyle='--', linewidth=1.5, label='Baseline (1.0x)')
    ax.grid(axis='x', linestyle='--', alpha=0.5)

    for bar in bars:
        w = bar.get_width()
        ax.annotate(f'{w:.2f}x',
                    xy=(w, bar.get_y() + bar.get_height() / 2),
                    xytext=(5, 0),
                    textcoords="offset points",
                    ha='left', va='center', fontsize=10, fontweight='bold')

    ax.legend(loc='lower right')
    fig.tight_layout()
    chart2_path = RESULTS_DIR / "benchmark_speedup.png"
    plt.savefig(chart2_path, dpi=200)
    plt.close()
    print(f"[+] Speedup chart saved to: {chart2_path}")

    # Chart 3: Head-to-Head C++ (AsmJit) vs Rust (Cranelift) if Rust data exists
    if has_rust:
        rust_jit_means = [b["rust_jit"]["mean"] for b in results]
        rust_jit_errs = [b["rust_jit"]["stdev"] for b in results]

        fig, ax = plt.subplots(figsize=(10, 6))
        w = 0.35
        r1 = ax.bar(x - w/2, cpp_jit_means, w, yerr=cpp_jit_errs, capsize=4,
                    label='C++ AsmJit JIT', color='#007ec6', alpha=0.9)
        r2 = ax.bar(x + w/2, rust_jit_means, w, yerr=rust_jit_errs, capsize=4,
                    label='Rust Cranelift JIT', color='#f74c00', alpha=0.9)

        ax.set_ylabel('Execution Time (seconds) - lower is better', fontsize=12)
        ax.set_title('Head-to-Head JIT Shootout: C++ (AsmJit) vs Rust (Cranelift)', fontsize=14, fontweight='bold')
        ax.set_xticks(x)
        ax.set_xticklabels(labels, rotation=15, ha='right', fontsize=11)
        ax.legend(fontsize=11)
        ax.grid(axis='y', linestyle='--', alpha=0.5)

        autolabel(r1)
        autolabel(r2)

        fig.tight_layout()
        chart3_path = RESULTS_DIR / "benchmark_vs_rust.png"
        plt.savefig(chart3_path, dpi=200)
        plt.close()
        print(f"[+] C++ vs Rust shootout chart saved to: {chart3_path}")

def generate_report(results, has_rust=False):
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    report_path = RESULTS_DIR / "BENCHMARK_REPORT.md"
    json_path = RESULTS_DIR / "benchmark_data.json"

    with open(json_path, "w") as f:
        json.dump(results, f, indent=2)

    with open(report_path, "w") as f:
        f.write("# Elysabettian Scientific Benchmark Report\n\n")
        f.write("Evaluation of runtime performance comparing C++ (AsmJit JIT) with Bytecode VM")
        if has_rust:
            f.write(" and Rust (Cranelift JIT).\n\n")
        else:
            f.write(".\n\n")

        f.write("### Methodology\n")
        f.write("- **Hardware Platform:** Host CPU native instruction execution.\n")
        f.write("- **Execution Rounds:** 2 warmup iterations followed by 5 timed evaluation runs per workload.\n")
        f.write(r"- **Statistical Metrics:** Sample Mean ($\mu$), Sample Standard Deviation ($\sigma$), and Speedup Factor ($\mu_{VM} / \mu_{JIT}$)." + "\n")
        f.write("- **Parity Verification:** Numerical floating-point equivalence across runtimes.\n\n")

        f.write("### C++ Benchmark Results (VM vs AsmJit JIT)\n\n")
        f.write("| Workload | Description | C++ VM Mean (s) | C++ JIT Mean (s) | Speedup | Parity |\n")
        f.write("| :--- | :--- | :---: | :---: | :---: | :---: |\n")

        for b in results:
            parity = "Verified" if b["cpp_parity"] else "FAILED"
            f.write(f"| **{b['name']}** | {b['desc']} | {b['cpp_vm']['mean']:.4f}s ± {b['cpp_vm']['stdev']:.4f} | {b['cpp_jit']['mean']:.4f}s ± {b['cpp_jit']['stdev']:.4f} | **{b['cpp_speedup']:.2f}x** | {parity} |\n")

        if has_rust:
            f.write("\n### Head-to-Head Comparison: C++ (AsmJit) vs Rust (Cranelift)\n\n")
            f.write("| Workload | C++ JIT (AsmJit) | Rust JIT (Cranelift) | Relative Advantage | Parity |\n")
            f.write("| :--- | :---: | :---: | :---: | :---: |\n")
            for b in results:
                c_t = b["cpp_jit"]["mean"]
                r_t = b["rust_jit"]["mean"]
                if c_t < r_t:
                    adv = f"C++ is **{r_t / c_t:.2f}x faster**"
                else:
                    adv = f"Rust is **{c_t / r_t:.2f}x faster**"
                parity = "Verified" if b["vs_parity"] else "FAILED"
                f.write(f"| **{b['name']}** | {c_t:.4f}s | {r_t:.4f}s | {adv} | {parity} |\n")

        f.write("\n### Visual Performance Charts\n\n")
        f.write("![Execution Times](benchmark_execution_times.png)\n\n")
        f.write("![Speedup Factors](benchmark_speedup.png)\n\n")
        if has_rust:
            f.write("![C++ vs Rust Shootout](benchmark_vs_rust.png)\n")

    print(f"[+] Markdown report saved to: {report_path}")
    print(f"[+] JSON raw data saved to: {json_path}")

def main():
    parser = argparse.ArgumentParser(description="Elysabettian Benchmark Suite")
    parser.add_argument("--vs-rust", action="store_true", default=True, help="Include Rust Cranelift binary in comparison")
    parser.add_argument("--no-rust", dest="vs_rust", action="store_false", help="Skip Rust comparison")
    parser.add_argument("--rust-bin", type=Path, default=RUST_RELEASE_BIN, help="Path to Rust ely_cli binary")
    args = parser.parse_args()

    cpp_bin = ensure_cpp_binary()
    has_rust = args.vs_rust and args.rust_bin.exists()

    if has_rust:
        print(f"[+] Found Rust binary at: {args.rust_bin}")
    else:
        print("[-] Rust binary not found or skipped. Benchmarking C++ only.")

    print("=" * 80)
    print(f"{'Elysabettian Benchmark Suite':^80}")
    print(f"{'5 Workloads | 2 Warmup Runs | 5 Timed Evaluations':^80}")
    print("=" * 80 + "\n")

    all_results = []

    for b in BENCHMARKS:
        bench_path = EXAMPLES_DIR / b["file"]
        if not bench_path.exists():
            print(f"[!] Warning: {bench_path} not found, skipping.")
            continue

        print(f"[*] Running workload: {b['name']} ({b['file']})")
        print(f"    Description: {b['desc']}")

        cpp_vm = benchmark_workload(cpp_bin, bench_path, "--no-jit")
        cpp_jit = benchmark_workload(cpp_bin, bench_path, "--jit")
        cpp_parity = check_parity(cpp_vm["output"], cpp_jit["output"])
        cpp_speedup = cpp_vm["mean"] / cpp_jit["mean"] if cpp_jit["mean"] > 0 else 0.0

        print(f"    C++ VM:   {cpp_vm['mean']:.4f}s ± {cpp_vm['stdev']:.4f}s")
        print(f"    C++ JIT:  {cpp_jit['mean']:.4f}s ± {cpp_jit['stdev']:.4f}s  (Speedup: {cpp_speedup:.2f}x)")

        rust_vm = None
        rust_jit = None
        vs_parity = True

        if has_rust:
            rust_vm = benchmark_workload(args.rust_bin, bench_path, "--no-jit")
            rust_jit = benchmark_workload(args.rust_bin, bench_path, "--jit")
            vs_parity = check_parity(cpp_jit["output"], rust_jit["output"])
            print(f"    Rust VM:  {rust_vm['mean']:.4f}s ± {rust_vm['stdev']:.4f}s")
            print(f"    Rust JIT: {rust_jit['mean']:.4f}s ± {rust_jit['stdev']:.4f}s")

        print(f"    Parity:   {'MATCH' if (cpp_parity and vs_parity) else 'MISMATCH'}\n")

        all_results.append({
            "id": b["id"],
            "name": b["name"],
            "desc": b["desc"],
            "file": b["file"],
            "cpp_vm": cpp_vm,
            "cpp_jit": cpp_jit,
            "cpp_parity": cpp_parity,
            "cpp_speedup": cpp_speedup,
            "rust_vm": rust_vm,
            "rust_jit": rust_jit,
            "vs_parity": vs_parity,
        })

    # Summary table
    print("=" * 80)
    if has_rust:
        print(f"{'Workload':<20} | {'C++ JIT (s)':<12} | {'Rust JIT (s)':<12} | {'Ratio':<10} | {'Parity'}")
        print("-" * 80)
        for res in all_results:
            c = res["cpp_jit"]["mean"]
            r = res["rust_jit"]["mean"]
            ratio_str = f"{r/c:.2f}x faster" if c < r else f"{c/r:.2f}x slower"
            p_str = "OK" if (res["cpp_parity"] and res["vs_parity"]) else "FAIL"
            print(f"{res['name']:<20} | {c:>8.4f}s   | {r:>8.4f}s   | {ratio_str:<10} | {p_str}")
    else:
        print(f"{'Workload':<20} | {'C++ VM (s)':<14} | {'C++ JIT (s)':<14} | {'Speedup':<10} | {'Parity'}")
        print("-" * 80)
        for res in all_results:
            p_str = "OK" if res["cpp_parity"] else "FAIL"
            print(f"{res['name']:<20} | {res['cpp_vm']['mean']:>6.4f}s ± {res['cpp_vm']['stdev']:<5.4f} | {res['cpp_jit']['mean']:>6.4f}s ± {res['cpp_jit']['stdev']:<5.4f} | {res['cpp_speedup']:>6.2f}x    | {p_str}")
    print("=" * 80 + "\n")

    generate_charts(all_results, has_rust=has_rust)
    generate_report(all_results, has_rust=has_rust)

if __name__ == "__main__":
    main()
