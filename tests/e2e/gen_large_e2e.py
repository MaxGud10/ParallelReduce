#!/usr/bin/env python3

import argparse
import math
import random
from pathlib import Path


def generate_values(count: int, value_type: str):
    if value_type == "int":
        return [random.randint(-1_000_000_000, 1_000_000_000) for _ in range(count)]

    if value_type == "double":
        return [random.uniform(-1_000_000_000.0, 1_000_000_000.0) for _ in range(count)]

    raise ValueError(f"unsupported type: {value_type}")


def compute_expected(values, value_type: str) -> str:
    if value_type == "int":
        return str(sum(values))

    if value_type == "double":
        return repr(math.fsum(values))

    raise ValueError(f"unsupported type: {value_type}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate large e2e test input and expected answer"
    )
    parser.add_argument(
        "--count",
        type=int,
        required=True,
        help="How many numbers to generate",
    )
    parser.add_argument(
        "--type",
        choices=["int", "double"],
        required=True,
        help="Value type",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Base output filename without extension, e.g. test_100",
    )
    parser.add_argument(
        "--output-dir",
        default="tests/e2e/generated",
        help="Directory where files will be written",
    )
    args = parser.parse_args()

    if args.count < 0:
        raise ValueError("--count must be >= 0")

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    values = generate_values(args.count, args.type)
    expected = compute_expected(values, args.type)

    input_path = output_dir / f"{args.output}.dat"
    answer_path = output_dir / f"{args.output}.ans"

    with input_path.open("w", encoding="utf-8") as f:
        f.write(f"{len(values)}\n")
        if values:
            f.write(" ".join(str(v) for v in values))
        f.write("\n")

    with answer_path.open("w", encoding="utf-8") as f:
        f.write(expected)
        f.write("\n")

    print(f"generated input : {input_path}")
    print(f"generated answer: {answer_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# python3 tests/e2e/gen_large_e2e.py --count 100 --type int --output test_100