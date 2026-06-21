# Building Bitmark — reproducible Ubuntu 24.04 build (experimental)

> **Status: experimental.** This is the first checked-in reproducible build of
> Bitmark for modern Linux. The underlying patches have been running on live
> nodes (locally and remotely) for roughly six months without issue, and the
> build below has been validated by a full chain reindex (see "Validation").
> It is published so people can build and run a working node today; it will be
> hardened and turned into a tagged release with prebuilt binaries in follow-up
> work.

## TL;DR

```bash
./build-docker.sh
# produces ./bitmarkd and ./bitmark-cli
```

This builds inside a pinned **Ubuntu 24.04 LTS** container and copies the two
binaries out.

## Why a pinned 24.04 build

Bitmark is old (Bitcoin ~0.10-era) C++. Modern toolchains have **silently
changed consensus results** for this code three separate times:

- **GCC 13+ miscompiles BMW256** (part of the LYRA2REv2 hash) at `-O2`, producing
  a wrong proof-of-work hash and stalling the chain at the multi-algorithm
  activation, **block 450,947**.
- **OpenSSL 3.x** rejects old non-canonical DER signatures, stalling at
  **block 605,359**.
- Plus assorted build breakage (Boost 1.74+, Berkeley DB 4.8, blake2 alignment).

A "newer is better" toolchain is therefore a *liability* for this codebase. We
deliberately pin **Ubuntu 24.04 LTS** — a settled GCC 13 / Boost 1.83 toolchain
whose consensus-affecting bugs are already fixed in-tree — rather than chase the
newest OS for the canonical build.

## ⚠️ Do NOT use `-Ofast` or `-march=native`

These re-introduce the GCC LYRA2REv2 miscompile and the chain stall returns. The
build uses default optimization with a protective per-file `-O1` on the hash
files. **Building with aggressive flags will fork you off the network.**

## Validation

The binary produced by this build was validated by a **full reindex of the
entire chain** against the existing block data:

- Reindexed **genesis → tip (height 2,399,930)** with **zero validation errors**.
- Crossed both historical stall points clean — **450,947** (LYRA2REv2 / GCC fix)
  and **605,359** (OpenSSL 3.x signatures).
- The reindexed **tip hash matches the live network** exactly, as do the hashes
  at both fork blocks. Because each block hash chains in its predecessor's, a
  matching tip hash means every block beneath it is identical to the network's.

Note: a normal reindex skips signature checks below the last checkpoint, so the
signature path is exercised structurally but not yet re-validated end-to-end. A
checkpoints-disabled signature re-validation is planned follow-up.

## Running the binary

The build links dynamically. On Ubuntu 24.04 it runs as-is. On newer releases
(e.g. 26.04, which ships Boost 1.90) it runs **if you supply the Boost 1.83
runtime libraries alongside it** via `LD_LIBRARY_PATH`. A fully static,
dependency-free binary is a later release-track item.

## Known limitations / follow-up

- The OpenSSL-3 signature parser (`#146`) is a bespoke lax-DER parser; replacing
  it with audited reference logic is planned.
- A startup proof-of-work known-answer self-test (to catch any future
  miscompile, on any compiler) is planned.
- Native Ubuntu 26.04 / Boost 1.90 build and prebuilt static binaries are a
  separate later track.
