#!/usr/bin/env python3

import argparse
import random
from pathlib import Path


def generate_values(size: int, value_type: str, distribution: str, seed: int):
    rng = random.Random(seed)

    if value_type == "int64":
        if distribution == "uniform_int":
            return [rng.randint(-1_000_000, 1_000_000) for _ in range(size)]
        if distribution == "ones":
            return [1] * size
        if distribution == "increasing":
            return list(range(size))
        if distribution == "decreasing":
            return list(range(size, 0, -1))
        if distribution == "small_range":
            return [rng.randint(-10, 10) for _ in range(size)]
        raise ValueError(f"unsupported distribution for int64: {distribution}")

    if value_type == "double":
        if distribution in {"uniform_float", "uniform_int"}:
            return [rng.uniform(-1000.0, 1000.0) for _ in range(size)]
        if distribution == "ones":
            return [1.0] * size
        if distribution == "increasing":
            return [i * 0.001 for i in range(size)]
        if distribution == "decreasing":
            return [(size - i) * 0.001 for i in range(size)]
        raise ValueError(f"unsupported distribution for double: {distribution}")

    raise ValueError(f"unsupported type: {value_type}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--size", type=int, required=True)
    parser.add_argument("--type", choices=["int64", "double"], default="int64")
    parser.add_argument(
        "--distribution",
        choices=["uniform_int", "ones", "increasing", "decreasing", "small_range", "uniform_float"],
        default="uniform_int",
    )
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    values = generate_values(args.size, args.type, args.distribution, args.seed)

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    with output.open("w", encoding="utf-8") as f:
        f.write(f"{len(values)}\n")
        f.write(" ".join(str(v) for v in values))
        f.write("\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())