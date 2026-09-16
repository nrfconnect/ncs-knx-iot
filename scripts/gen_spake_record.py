#!/usr/bin/env python3
"""Generate a C SPAKE2+ verifier record for a KNX IoT application.

The Point API repository owns the cryptographic implementation.
This wrapper supplies the configured password to its offline generator and emits the record consumed by the add-on.
"""

import argparse
import importlib.util
import re
import sys
from pathlib import Path


def load_generator(path: Path):
    """Load the Point API stack's offline SPAKE2+ generator."""
    if not path.is_file():
        raise FileNotFoundError(f"SPAKE2+ generator not found: {path}")

    sys.path.insert(0, str(path.parent))
    try:
        spec = importlib.util.spec_from_file_location("knx_spake_generator", path)
        if spec is None or spec.loader is None:
            raise ImportError(f"cannot load SPAKE2+ generator: {path}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    finally:
        sys.path.pop(0)

    return module


def format_bytes(data: bytes) -> str:
    """Format bytes as wrapped C initializer values."""
    lines = []
    for offset in range(0, len(data), 12):
        chunk = data[offset : offset + 12]
        lines.append("    " + ", ".join(f"0x{value:02x}" for value in chunk))
    return ",\n".join(lines)


def format_source(
    symbol: str, w0: bytes, point_l: bytes, salt: bytes, iterations: int
) -> str:
    """Format a complete C definition for an SPAKE2+ verifier record."""
    return f"""#include "oc_knx.h"

const oc_spake_record_t {symbol} = {{
  .w0 = {{
{format_bytes(w0)}
  }},
  .L = {{
{format_bytes(point_l)}
  }},
  .salt = {{
{format_bytes(salt)}
  }},
  .it = {iterations},
  .valid = true,
}};
"""


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a KNX IoT SPAKE2+ verifier record."
    )
    parser.add_argument("--stack-generator", type=Path, required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if not 6 <= len(args.password) <= 32:
        parser.error("password must contain between 6 and 32 characters")
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", args.symbol) is None:
        parser.error("symbol must be a valid C identifier")

    try:
        generator = load_generator(args.stack_generator)
        salt = generator.SALT
        iterations = generator.ITERATIONS
        w0, w1, point_l = generator._derive_record(args.password, salt, iterations)
        generator._verify_record(
            args.password, salt, iterations, w0, w1, point_l
        )

        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            format_source(args.symbol, w0, point_l, salt, iterations),
            encoding="utf-8",
        )
    except (AssertionError, AttributeError, ImportError, OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
