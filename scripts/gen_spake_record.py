#!/usr/bin/env python3
"""Generate a C SPAKE2+ verifier record for a KNX IoT application.

Derives the offline registration record (w0, L = w1 * G) for the configured password, as specified in
RFC 9383 section 3.2 with the KNX IoT PBKDF2 parameters, and emits the record consumed by the add-on.
"""

import argparse
import hashlib
import re
import struct
from pathlib import Path

# Development salt and iteration count of the record, KNX IoT 3-10-5 3.6.6.2 (16 to 32 byte salt).
SALT = bytes.fromhex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
ITERATIONS = 50000

# KNX IoT PASE uses empty prover and verifier identities.
ID_PROVER = b""
ID_VERIFIER = b""

# NIST P-256 domain parameters.
P256_P = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF
P256_A = P256_P - 3
P256_B = 0x5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B
P256_N = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
P256_G = (
    0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296,
    0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5,
)


def encode_length_prefixed(data: bytes) -> bytes:
    """Encode data with its 64-bit little-endian length, RFC 9383 section 3.2."""
    return struct.pack("<Q", len(data)) + data


def point_add(p, q):
    """Add two affine P-256 points, None being the point at infinity."""
    if p is None:
        return q
    if q is None:
        return p
    if p[0] == q[0] and (p[1] + q[1]) % P256_P == 0:
        return None
    if p == q:
        slope = (3 * p[0] * p[0] + P256_A) * pow(2 * p[1], -1, P256_P)
    else:
        slope = (q[1] - p[1]) * pow(q[0] - p[0], -1, P256_P)
    slope %= P256_P
    x = (slope * slope - p[0] - q[0]) % P256_P
    return x, (slope * (p[0] - x) - p[1]) % P256_P


def scalar_mult(scalar: int, point):
    """Multiply an affine P-256 point by a scalar (double-and-add)."""
    result = None
    scalar %= P256_N
    while scalar:
        if scalar & 1:
            result = point_add(result, point)
        point = point_add(point, point)
        scalar >>= 1
    return result


def is_on_curve(point) -> bool:
    """Check that an affine point satisfies the P-256 curve equation."""
    x, y = point
    return (y * y - (x * x * x + P256_A * x + P256_B)) % P256_P == 0


def derive_record(password: str, salt: bytes, iterations: int):
    """Derive (w0, L) from the password: PBKDF2-HMAC-SHA256 to 80 bytes, split, reduced mod n."""
    pbkdf2_input = (
        encode_length_prefixed(password.encode("utf-8"))
        + encode_length_prefixed(ID_PROVER)
        + encode_length_prefixed(ID_VERIFIER)
    )
    output = hashlib.pbkdf2_hmac("sha256", pbkdf2_input, salt, iterations, dklen=80)

    w0 = int.from_bytes(output[:40], "big") % P256_N
    w1 = int.from_bytes(output[40:], "big") % P256_N
    if w0 == 0 or w1 == 0:
        raise ValueError("password derives a zero SPAKE2+ scalar")

    point_l = scalar_mult(w1, P256_G)
    if point_l is None or not is_on_curve(point_l):
        raise ValueError("L is not a valid P-256 point")

    # The verifier computes y * L, the prover w1 * (y * G); both must agree.
    y = 0x00AABBCCDDEEFF00112233445566778899AABBCCDDEEFF00112233445566778899
    if scalar_mult(y, point_l) != scalar_mult(w1, scalar_mult(y, P256_G)):
        raise ValueError("L is not consistent with w1")

    encoded_l = b"\x04" + point_l[0].to_bytes(32, "big") + point_l[1].to_bytes(32, "big")
    return w0.to_bytes(32, "big"), encoded_l


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
    parser.add_argument("--password", required=True)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if not 6 <= len(args.password) <= 32:
        parser.error("password must contain between 6 and 32 characters")
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", args.symbol) is None:
        parser.error("symbol must be a valid C identifier")

    try:
        w0, point_l = derive_record(args.password, SALT, ITERATIONS)

        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            format_source(args.symbol, w0, point_l, SALT, ITERATIONS),
            encoding="utf-8",
        )
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
