#!/usr/bin/env python3
"""
HPM EXIP Boot Image Encryption Tool

Replicates the functionality of hpm_img_util.exe (create_image) in pure Python.
Produces an encrypted XIP boot image compatible with HPMicro HPM5300 BootROM EXIP.

Final file (after EXIP prepend +0x400):
  0x0000-0x03FF   EXIP KEK blob
  0x0400+         Inner boot image (plaintext except EXIP ciphertext zone)

Inner boot image (build_boot_image; same addressing as hpm_image_helper):
  0x0000          Flash word + 0xFF pad up to hdr_offset
  hdr_offset      Boot container: boot_header + fw_info (工具「固件容器首地址相对镜像」= 0x0C00)
  fw_offset       Application load area (工具「固件首地址相对容器」配置值 = 0x2000，写入 fw_info.offset；
                  线性布局下即镜像内从 0 起的偏移 0x2000，中间 0x0C00..0x2000 为填充，勿与 VA 相加混淆)
  EXIP VA 0x80003000 => inner off (exip_start - mem_base) typically 0x3000

EXIP AES key / 8-byte nonce 来源（与 HPM 镜像助手对齐时必读）:
  - 本脚本若未指定 --fixed-exip-material 或 --rng-seed，则对每个 EXIP 区使用 os.urandom，
    每次构建的前 0x100（KEK-ECB 保护的配置区）都会变化。
  - 镜像助手界面上「区域 AES Key / Nonce」通常保持你填入的值；同一原始固件多次生成时
    前 0x100 相同，说明工具侧不是「每次真随机」，而是固定或项目内保存的值。
  - 要与助手逐字节一致：把界面上的 Key（32 hex）+ Nonce（16 hex）连成 48 hex，
    传给 --fixed-exip-material（makefile: ENCRYPT_FIXED_EXIP_MATERIAL）。

References:
  - hpm_bootheader.h  (boot_header_t / fw_info_table_t)
  - hpm_romapi.h      (exip_region_param_t)
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import struct
import sys
from typing import Callable, Sequence

try:
    from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
except ImportError:
    sys.exit(
        "ERROR: 'cryptography' package is required.\n"
        "       Install with:  pip install cryptography"
    )

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
BOOTHEADER_TAG = 0xBF
BOOTHEADER_VERSION = 0x10
FW_TYPE_APP = 0
HASH_TYPE_SHA256 = 1
FLASH_CFG_SIZE = 0x10
# 16 字节 Flash Word 后留 4 字节 0，再从镜像内 0x14 起填 FF → 最终文件里 FF 从 0x400+0x14=0x414 起（与 HPM 工具一致）
FLASH_CFG_TAIL_ZERO = 4
EXIP_BLOB_SIZE = 0x400
# HPM Windows 工具：仅前 0x100 字节经 KEK-ECB；0x100..0x3FF 输出为全 0（不加密尾部零填充）
EXIP_BLOB_KEK_PLAINTEXT_LEN = 0x100
BOOT_HEADER_SIZE = 0x90  # 16 bytes header + 128 bytes fw_info (1 entry)
FW_INFO_SIZE = 0x80
# Linked demo.bin: .mg_pack_ver at VA 0x80003200 ⇒ file offset 0x2E00 (makefile MG_VER_FILE_OFFSET)
DEFAULT_VER_PATCH_OFFSET = 0x2E00


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def _parse_int(s: str) -> int:
    """Parse an integer from string, supporting 0x prefix."""
    return int(s, 0)


def parse_board_versions(header_path: str) -> tuple[int, int]:
    """Parse BOARD_FW_VERSION and BOARD_HW_VERSION from a C header file."""
    text = open(header_path, encoding="utf-8", errors="ignore").read()
    fw = hw = 0
    m = re.search(r"#define\s+BOARD_FW_VERSION\s+\((\d+)\)", text)
    if m:
        fw = int(m.group(1))
    m = re.search(r"#define\s+BOARD_HW_VERSION\s+\((\d+)\)", text)
    if m:
        hw = int(m.group(1))
    return fw, hw


def _pad_to(data: bytes, size: int, fill: int = 0xFF) -> bytes:
    if len(data) >= size:
        return data[:size]
    return data + bytes([fill]) * (size - len(data))


def aes_ecb_encrypt(key: bytes, plaintext: bytes) -> bytes:
    assert len(plaintext) % 16 == 0
    cipher = Cipher(algorithms.AES(key), modes.ECB())
    enc = cipher.encryptor()
    return enc.update(plaintext) + enc.finalize()


def aes_ecb_decrypt(key: bytes, ciphertext: bytes) -> bytes:
    assert len(ciphertext) % 16 == 0
    cipher = Cipher(algorithms.AES(key), modes.ECB())
    dec = cipher.decryptor()
    return dec.update(ciphertext) + dec.finalize()


def _seed_to_bytes(seed_arg: str) -> bytes:
    """--rng-seed: plain string as UTF-8, or hex:DEADBEEF... for raw bytes."""
    s = seed_arg.strip()
    if s.lower().startswith("hex:"):
        return bytes.fromhex(s[4:].strip())
    return s.encode("utf-8")


def make_seeded_randbytes(seed: bytes) -> Callable[[int], bytes]:
    """
    Deterministic byte stream from seed (SHA-256 counter expansion).
    Same seed + same region order => same AES keys/nonces => reproducible *_secrit.bin.
    """
    counter = [0]

    def randbytes(n: int) -> bytes:
        out = bytearray()
        while len(out) < n:
            block = hashlib.sha256(
                b"HPMEXIP\x00" + seed + struct.pack("<Q", counter[0])
            ).digest()
            counter[0] += 1
            out.extend(block)
        return bytes(out[:n])

    return randbytes


def make_fixed_material_randbytes(hex_material: str) -> Callable[[int], bytes]:
    """
    Fixed key/nonce per EXIP region from hex: 48 hex chars per region
    (16-byte AES key + 8-byte CTR nonce). Regions consume pairs in order.
    """
    data = bytes.fromhex(hex_material.replace(" ", ""))
    if len(data) == 0 or len(data) % 24 != 0:
        sys.exit(
            "ERROR: --fixed-exip-material must be non-empty hex with length "
            "multiple of 24 bytes (48 hex digits per EXIP region)"
        )
    pairs: list[tuple[bytes, bytes]] = []
    for j in range(0, len(data), 24):
        pairs.append((data[j : j + 16], data[j + 16 : j + 24]))
    ri = [0]
    phase = [0]  # 0 -> return key, 1 -> return nonce then next region

    def randbytes(n: int) -> bytes:
        if ri[0] >= len(pairs):
            sys.exit("ERROR: --fixed-exip-material exhausted (more regions than key/nonce pairs)")
        key, nonce = pairs[ri[0]]
        if n == 16:
            if phase[0] != 0:
                sys.exit("ERROR: internal --fixed-exip-material phase (expected 16-byte key)")
            phase[0] = 1
            return key
        if n == 8:
            if phase[0] != 1:
                sys.exit("ERROR: internal --fixed-exip-material phase (expected 8-byte nonce)")
            phase[0] = 0
            ri[0] += 1
            return nonce
        sys.exit(f"ERROR: unsupported randbytes({n}) for --fixed-exip-material")

    return randbytes


def _aes_ctr_iv_from_nonce(nonce: bytes, layout: str) -> bytes:
    """
    Build 16-byte initial counter block for PyCA AES-CTR (OpenSSL-style 128-bit increment).

    layout:
      nonce_high — IV = nonce || 0^8  (default; exip_region_param_t.ctr as high 64 bits)
      nonce_low  — IV = 0^8 || nonce (some tools / docs use ctr in low half; try when diffing)
    """
    if len(nonce) != 8:
        raise ValueError("nonce must be 8 bytes")
    if layout == "nonce_high":
        return nonce + b"\x00" * 8
    if layout == "nonce_low":
        return b"\x00" * 8 + nonce
    raise ValueError(f"unknown ctr_iv_layout {layout!r}")


def aes_ctr_encrypt(
    key: bytes, nonce: bytes, plaintext: bytes, *, ctr_iv_layout: str = "nonce_high"
) -> bytes:
    """AES-128-CTR for EXIP payload; ctr_iv_layout selects 16-byte IV from 8-byte ctr/nonce."""
    iv = _aes_ctr_iv_from_nonce(nonce, ctr_iv_layout)
    cipher = Cipher(algorithms.AES(key), modes.CTR(iv))
    enc = cipher.encryptor()
    return enc.update(plaintext) + enc.finalize()


def aes_ctr_decrypt(
    key: bytes, nonce: bytes, ciphertext: bytes, *, ctr_iv_layout: str = "nonce_high"
) -> bytes:
    """AES-128-CTR decrypt (symmetric XOR stream)."""
    iv = _aes_ctr_iv_from_nonce(nonce, ctr_iv_layout)
    cipher = Cipher(algorithms.AES(key), modes.CTR(iv))
    dec = cipher.decryptor()
    return dec.update(ciphertext) + dec.finalize()


# ---------------------------------------------------------------------------
# Boot image builder
# ---------------------------------------------------------------------------
def extract_raw_firmware(boot_image: bytes, fw_offset: int) -> bytes:
    """
    If the input already contains a flash config + boot header (from SES build),
    extract the raw firmware portion starting at fw_offset.
    """
    if len(boot_image) <= fw_offset:
        return boot_image
    tag_byte = boot_image[0x0C00] if len(boot_image) > 0x0C00 else 0
    if tag_byte == BOOTHEADER_TAG:
        return boot_image[fw_offset:]
    flash_tag = struct.unpack_from("<I", boot_image, 0)[0]
    if (flash_tag & 0xFFFF0000) == 0xFCF90000:
        return boot_image[fw_offset:]
    return boot_image


def build_fw_info(
    fw_data: bytes,
    fw_offset: int,
    load_addr: int,
    entry: int,
) -> bytes:
    """Build a fw_info_table_t (128 bytes)."""
    sha = hashlib.sha256(fw_data).digest()
    flags = (FW_TYPE_APP & 0xF) | ((HASH_TYPE_SHA256 & 0xF) << 8)
    buf = struct.pack(
        "<IIII",
        fw_offset,
        len(fw_data),
        flags,
        0,
    )
    buf += struct.pack("<I I I I", load_addr, 0, entry, 0)
    buf += sha.ljust(64, b"\x00")
    buf += b"\x00" * 32  # iv
    assert len(buf) == FW_INFO_SIZE
    return buf


def build_boot_header(
    fw_info: bytes,
    hdr_offset: int,
    sw_version: int,
    fuse_version: int = 0,
) -> bytes:
    """Build boot_header_t + fw_info (total 0x90 bytes) at hdr_offset."""
    hdr = struct.pack(
        "<BBH I HBB HH",
        BOOTHEADER_TAG,
        BOOTHEADER_VERSION,
        BOOT_HEADER_SIZE,
        0,  # flags
        sw_version,
        fuse_version,
        1,  # fw_count
        0,  # dc_block_offset
        0,  # sig_block_offset
    )
    assert len(hdr) == 0x10
    return hdr + fw_info


def build_flash_config(opts: Sequence[int]) -> bytes:
    """Build the 16-byte flash config option block."""
    return struct.pack("<4I", *opts)


def build_boot_image(
    fw_data: bytes,
    fw_offset: int,
    hdr_offset: int,
    load_addr: int,
    entry: int,
    sw_version: int,
    flash_opts: Sequence[int],
) -> bytes:
    """
    Assemble a complete boot image (flash_cfg + boot_header + firmware).
    Layout:
      0x0000      flash config (16 bytes + padding to hdr_offset)
      hdr_offset  boot container: boot_header + fw_info (0x90 bytes)
      fw_offset   firmware data (fw_offset > hdr_offset; gap is 0x00 fill)
    Total size is padded to 16-byte alignment (matching hpm_img_util behaviour).
    Padding matches HPM Windows tool: FLASH_CFG_TAIL_ZERO bytes of 0 after the 16-byte
    flash word, then 0xFF up to hdr_offset (file offset 0x414..0xFFF when +0x400 EXIP);
    0x00 from after boot header to fw_offset.
    """
    raw_size = fw_offset + len(fw_data)
    total_size = (raw_size + 15) & ~15
    image = bytearray(b"\x00" * total_size)

    flash_cfg = build_flash_config(flash_opts)
    image[0 : len(flash_cfg)] = flash_cfg
    ff_start = len(flash_cfg) + FLASH_CFG_TAIL_ZERO
    if hdr_offset > ff_start:
        image[ff_start : hdr_offset] = b"\xff" * (hdr_offset - ff_start)

    fw_info = build_fw_info(fw_data, fw_offset, load_addr, entry)
    boot_hdr = build_boot_header(fw_info, hdr_offset, sw_version)
    image[hdr_offset : hdr_offset + len(boot_hdr)] = boot_hdr

    image[fw_offset : fw_offset + len(fw_data)] = fw_data

    return bytes(image)


# ---------------------------------------------------------------------------
# EXIP encryption
# ---------------------------------------------------------------------------
def build_exip_blob(
    kek: bytes,
    regions: list[tuple[int, int, bytes, bytes]],
) -> bytes:
    """
    Build and encrypt the 1024-byte EXIP config blob.

    Each region entry is (start_addr, length, aes_key_16, nonce_8).
    Up to 4 regions; unused slots are zero-filled.
    Only the first EXIP_BLOB_KEK_PLAINTEXT_LEN (256) bytes are KEK-AES-ECB encrypted;
    bytes 0x100..0x3FF in the output are zero (matches HPM Windows image tool).

    Plaintext layout per region (32 bytes):
      [0:4]   start address (LE)
      [4:8]   length (LE)
      [8:24]  AES-128 key
      [24:32] CTR nonce (8 bytes)
    4 regions = 128 bytes, then zero to 256 bytes before ECB.
    """
    plaintext = bytearray(EXIP_BLOB_KEK_PLAINTEXT_LEN)
    for i, (start, length, key, nonce) in enumerate(regions[:4]):
        off = i * 32
        struct.pack_into("<II", plaintext, off, start, length)
        plaintext[off + 8 : off + 24] = key
        plaintext[off + 24 : off + 32] = nonce

    enc_prefix = aes_ecb_encrypt(kek, bytes(plaintext))
    assert len(enc_prefix) == EXIP_BLOB_KEK_PLAINTEXT_LEN
    return enc_prefix + b"\x00" * (EXIP_BLOB_SIZE - EXIP_BLOB_KEK_PLAINTEXT_LEN)


def encrypt_exip_image(
    boot_image: bytes,
    kek: bytes,
    mem_base: int,
    regions: list[tuple[int, int]],
    randbytes: Callable[[int], bytes] | None = None,
    ctr_iv_layout: str = "nonce_high",
) -> bytes:
    """
    Apply EXIP encryption to a boot image.

    Steps:
      1. For each EXIP region, obtain AES-128 key + 8-byte nonce (see randbytes below)
      2. Encrypt the firmware data in-place using AES-128-CTR
      3. Build the EXIP config blob (encrypted with KEK)
      4. Prepend the blob (shifting the entire image by 0x400)

    randbytes:
      If None, uses os.urandom (unique each run). This is not equivalent to HPM 镜像助手
      when the GUI keeps fixed Key/Nonce — use make_fixed_material_randbytes(...) with
      the same hex as the tool UI, or make_seeded_randbytes(...) for tests.
    """
    image = bytearray(boot_image)
    exip_regions: list[tuple[int, int, bytes, bytes]] = []
    rb = randbytes if randbytes is not None else os.urandom

    for region_start, region_len in regions:
        aes_key = rb(16)
        nonce = rb(8)

        file_offset = region_start - mem_base
        end_offset = file_offset + region_len

        chunk = bytes(image[file_offset:end_offset])
        if len(chunk) < region_len:
            chunk = chunk.ljust(region_len, b"\x00")

        encrypted_chunk = aes_ctr_encrypt(
            aes_key, nonce, chunk, ctr_iv_layout=ctr_iv_layout
        )
        image[file_offset : file_offset + len(encrypted_chunk)] = encrypted_chunk

        exip_regions.append((region_start, region_len, aes_key, nonce))

    exip_blob = build_exip_blob(kek, exip_regions)
    return exip_blob + bytes(image)


def _region_label(
    file_off: int,
    *,
    hdr_offset: int,
    fw_offset: int,
    mem_base: int,
    exip_start: int,
    exip_len: int,
) -> str:
    """Classify a byte offset in the final encrypted file (EXIP blob + boot_image)."""
    if file_off < EXIP_BLOB_SIZE:
        return "exip_kek_blob"
    hi = file_off - EXIP_BLOB_SIZE
    if hi < hdr_offset:
        return "flash_cfg_or_ff_pad"
    if hi < hdr_offset + BOOT_HEADER_SIZE:
        return "boot_header_plaintext"
    if hi < fw_offset:
        return "pad_header_to_fw"
    ex_fo = exip_start - mem_base
    ex_end = ex_fo + exip_len
    if hi < ex_fo:
        return "fw_plaintext_before_exip_region"
    if hi < ex_end:
        return "fw_exip_ciphertext"
    return "fw_after_exip_or_padding"


def diff_encrypted_bins(
    path_a: str,
    path_b: str,
    *,
    hdr_offset: int,
    fw_offset: int,
    mem_base: int,
    exip_start: int,
    exip_len: int,
) -> None:
    with open(path_a, "rb") as f:
        a = f.read()
    with open(path_b, "rb") as f:
        b = f.read()
    print(f"[DIFF] A={path_a} ({len(a)} bytes)")
    print(f"[DIFF] B={path_b} ({len(b)} bytes)")
    fw_sz_off = EXIP_BLOB_SIZE + hdr_offset + 0x10 + 4
    for tag, buf in ("A", a), ("B", b):
        if len(buf) >= fw_sz_off + 4:
            sz = struct.unpack_from("<I", buf, fw_sz_off)[0]
            print(f"[DIFF] {tag} fw_info.size @0x{fw_sz_off:x} = 0x{sz:x} ({sz})")
        else:
            print(f"[DIFF] {tag} too short for fw_info.size @0x{fw_sz_off:x}")
    if len(a) != len(b):
        print(f"[DIFF] length differs: {len(a)} vs {len(b)} (first compare min length)")
    elif a != b:
        nd = sum(1 for i in range(len(a)) if a[i] != b[i])
        print(f"[DIFF] differing bytes (same length): {nd} / {len(a)}")
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            lo = max(0, i - 4)
            hi = min(n, i + 12)
            reg = _region_label(
                i,
                hdr_offset=hdr_offset,
                fw_offset=fw_offset,
                mem_base=mem_base,
                exip_start=exip_start,
                exip_len=exip_len,
            )
            print(f"[DIFF] first mismatch at file offset 0x{i:x} ({i}) region={reg}")
            print(f"[DIFF]  A[{lo:x}:{hi:x}] = {a[lo:hi].hex()}")
            print(f"[DIFF]  B[{lo:x}:{hi:x}] = {b[lo:hi].hex()}")
            return
    if len(a) != len(b):
        print("[DIFF] prefix identical; lengths differ only past shared prefix")
    else:
        print("[DIFF] files are identical")


def run_selftest() -> None:
    """
    Sanity check: KEK/flash/EXIP 与 makefile / imageutil.json 一致；sw_version 从合成 bin 的
    DEFAULT_VER_PATCH_OFFSET 读出（与 main() 读 input bin 相同），不写死某个版本号。
    """
    kek = bytes.fromhex("1ca7333ade25f2ca7b468fae642445a3")
    fixed_material = "00112233445566778899aabbccddeeff0102030405060708"
    fw_offset = 0x2000
    hdr_offset = 0x0C00
    load_addr = 0x80003000
    entry = 0x80003000
    mem_base = 0x80000000
    exip_start = 0x80003000
    exip_len = 0x10000
    ver_off = DEFAULT_VER_PATCH_OFFSET
    flash_opts = [0xFCF90002, 0x00000006, 0x00001000, 0x00000000]

    fo = exip_start - mem_base
    need_fw = fo + exip_len - fw_offset
    raw_len = max(fw_offset + need_fw, ver_off + 4)
    raw_input = bytearray(raw_len)
    raw_input[hdr_offset] = BOOTHEADER_TAG
    for i in range(fw_offset, raw_len):
        raw_input[i] = (i * 17 + 0x5A) & 0xFF
    # 任意测试版本（非固定量产号）；真实构建由 input bin 该偏移决定 sw_version
    example_fw, example_hw = 0xABCD, 2
    struct.pack_into("<HH", raw_input, ver_off, example_fw, example_hw)
    sw_version = struct.unpack_from("<H", raw_input, ver_off)[0]
    if sw_version != example_fw:
        raise AssertionError("selftest sw_version readback")
    fw_data = extract_raw_firmware(bytes(raw_input), fw_offset)
    plain = build_boot_image(
        fw_data, fw_offset, hdr_offset, load_addr, entry, sw_version, flash_opts
    )

    for layout in ("nonce_high", "nonce_low"):
        rb = make_fixed_material_randbytes(fixed_material)
        enc = encrypt_exip_image(
            plain,
            kek,
            mem_base,
            [(exip_start, exip_len)],
            randbytes=rb,
            ctr_iv_layout=layout,
        )
        if len(enc) != EXIP_BLOB_SIZE + len(plain):
            raise AssertionError("encrypted length mismatch")

        blob = enc[:EXIP_BLOB_SIZE]
        tail = memoryview(enc)[EXIP_BLOB_SIZE:]
        if bytes(blob[EXIP_BLOB_KEK_PLAINTEXT_LEN:]) != b"\x00" * (
            EXIP_BLOB_SIZE - EXIP_BLOB_KEK_PLAINTEXT_LEN
        ):
            raise AssertionError("EXIP blob tail should be zero")

        pt_blob = aes_ecb_decrypt(kek, blob[:EXIP_BLOB_KEK_PLAINTEXT_LEN])
        st, ln = struct.unpack_from("<II", pt_blob, 0)
        if st != exip_start or ln != exip_len:
            raise AssertionError(
                f"blob r0: start=0x{st:x} len=0x{ln:x} != 0x{exip_start:x} 0x{exip_len:x}"
            )
        k = bytes(pt_blob[8:24])
        n = bytes(pt_blob[24:32])
        ct = bytes(tail[fo : fo + exip_len])
        exp = plain[fo : fo + exip_len]
        if aes_ctr_decrypt(k, n, ct, ctr_iv_layout=layout) != exp:
            raise AssertionError(f"CTR roundtrip failed layout={layout}")

        rb2 = make_fixed_material_randbytes(fixed_material)
        enc2 = encrypt_exip_image(
            plain,
            kek,
            mem_base,
            [(exip_start, exip_len)],
            randbytes=rb2,
            ctr_iv_layout=layout,
        )
        if enc != enc2:
            raise AssertionError(f"determinism failed layout={layout}")

    print(
        f"[SELFTEST] OK: sw_version=0x{sw_version:x} from bin off 0x{ver_off:x}; "
        "KEK blob + CTR roundtrip (nonce_high/low); fixed material deterministic"
    )


# ---------------------------------------------------------------------------
# HPM tool delegation (hpm_img_util.exe -- create_image)
# ---------------------------------------------------------------------------
def _run_hpm_tool_encrypt(args: argparse.Namespace) -> None:
    """
    Generate imageutil.json from CLI args, then call hpm_img_util.exe to
    produce the encrypted EXIP image.  Works on Windows (native) and
    Linux (via wine — install with ``apt install wine``).
    """
    import json
    import subprocess

    hpm_tool = args.use_hpm_tool
    if not os.path.isfile(hpm_tool):
        sys.exit(f"ERROR: hpm_img_util not found at {hpm_tool}")

    fw_offset = _parse_int(args.fw_offset)
    hdr_offset = _parse_int(args.hdr_offset)
    load_addr = _parse_int(args.load_addr)
    entry = _parse_int(args.entry)
    mem_base = _parse_int(args.mem_base)
    exip_start = _parse_int(args.exip_start)
    exip_len = _parse_int(args.exip_len)
    sw_version = _parse_int(args.sw_version)
    flash_opts = [_parse_int(x.strip()) for x in args.flash_opts.split(",")]
    input_path = os.path.abspath(args.input).replace("\\", "/")
    output_path = os.path.abspath(args.output).replace("\\", "/")

    ver_off = _parse_int(args.ver_patch_offset)
    if args.board_header:
        board_fw, board_hw = parse_board_versions(args.board_header)
        with open(args.input, "r+b") as f:
            raw = bytearray(f.read())
            struct.pack_into("<HH", raw, ver_off, board_fw, board_hw)
            f.seek(0)
            f.write(raw)
            f.truncate()
        sw_version = board_fw
        print(f"[HPM-TOOL] Patched version: FW={board_fw} HW={board_hw}")
    else:
        with open(args.input, "rb") as f:
            raw = f.read()
        if len(raw) >= ver_off + 4:
            sw_version = struct.unpack_from("<H", raw, ver_off)[0]

    region = [f"0x{exip_start:x}", f"0x{exip_len:x}"]
    if args.fixed_exip_material is not None:
        mat = bytes.fromhex(args.fixed_exip_material.replace(" ", ""))
        if len(mat) >= 24:
            region.append(mat[:16].hex())
            region.append(mat[16:24].hex())

    cfg = {
        "boot_image": {
            "firmware_info": [{
                "binary_path": input_path,
                "entry_point": f"0x{entry:x}",
                "fw_type": "application",
                "hash_type": "sha256",
                "is_encrypted": False,
                "load_addr": f"0x{load_addr:x}",
                "offset": f"0x{fw_offset:x}",
            }],
            "header": {
                "fuse_version": 0,
                "offset": f"0x{hdr_offset:x}",
                "sw_version": sw_version,
            },
        },
        "exip_image_info": {
            "kek": args.kek,
            "mem_base_addr": f"0x{mem_base:x}",
            "regions": [region],
        },
        "flash_cfg": {
            "cfg_option": [f"0x{v:08x}" for v in flash_opts],
            "offset": "0x0000",
        },
        "output_path": output_path,
    }

    json_path = output_path + ".imageutil.json"
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=4)
    print(f"[HPM-TOOL] Config: {json_path}")

    use_wine = (
        not sys.platform.startswith("win")
        and hpm_tool.lower().endswith(".exe")
    )
    cmd = (["wine"] if use_wine else []) + [
        hpm_tool, "--", "create_image", json_path,
    ]
    print(f"[HPM-TOOL] {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    for line in (result.stdout or "").strip().splitlines():
        print(f"[HPM-TOOL]   {line}")
    if result.returncode != 0:
        if result.stderr:
            print(f"[HPM-TOOL] stderr: {result.stderr.strip()}")
        sys.exit(f"ERROR: hpm_img_util exited with code {result.returncode}")

    if not os.path.isfile(output_path):
        sys.exit(f"ERROR: output {output_path} not created")

    print(f"[HPM-TOOL] Output: {output_path} ({os.path.getsize(output_path)} bytes)")
    try:
        os.remove(json_path)
    except OSError:
        pass


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> None:
    ap = argparse.ArgumentParser(
        description="HPM EXIP boot image encryption (pure Python)"
    )
    ap.add_argument(
        "--selftest",
        action="store_true",
        help="Run built-in ECB/CTR roundtrip; no --input/--output/--kek.",
    )
    ap.add_argument(
        "--diff-bin",
        nargs=2,
        metavar=("A", "B"),
        default=None,
        help="Compare two encrypted images; prints first differing offset and region name. "
        "Uses --hdr-offset/--fw-offset/--mem-base/--exip-* for region labels.",
    )
    ap.add_argument("--input", default=None, help="Input firmware .bin")
    ap.add_argument("--output", default=None, help="Output encrypted .bin")
    ap.add_argument("--kek", default=None, help="KEK hex string (32 hex chars)")
    ap.add_argument(
        "--fw-offset",
        default="0x2000",
        help="App area start in inner image (= fw_info.offset; tool: 固件相对容器的偏移 0x2000)",
    )
    ap.add_argument(
        "--fw-length",
        default=None,
        help="After extract_raw_firmware, truncate firmware payload to this many bytes "
        "(dec or 0x hex). Use the same value as hpm_image_helper Firmware Info 'Size' "
        "when objcopy/demo.bin is longer than the tool's effective app length.",
    )
    ap.add_argument(
        "--append-hpm-image-tail",
        action="store_true",
        help="After --fw-length, append the next hdr_offset bytes from the input bin "
        "(from raw offset fw_offset+fw_length). Matches HPM image helper: extra tail "
        "length equals container header offset (typically 0xC00). Requires --fw-length.",
    )
    ap.add_argument(
        "--hdr-offset",
        default="0x0c00",
        help="Boot container start in inner image (tool: 容器相对镜像 0x0C00)",
    )
    ap.add_argument("--load-addr", default="0x80003000", help="Load address")
    ap.add_argument("--entry", default="0x80003000", help="Entry point")
    ap.add_argument("--mem-base", default="0x80000000", help="XPI memory base address")
    ap.add_argument("--exip-start", default="0x80003000", help="EXIP region start")
    ap.add_argument("--exip-len", default="0x10000", help="EXIP region length")
    ap.add_argument("--sw-version", default="0", help="Software version")
    ap.add_argument(
        "--flash-opts",
        default="0xfcf90002,0x00000006,0x00001000,0x00000000",
        help="Flash config options (4 comma-separated hex values)",
    )
    ap.add_argument(
        "--board-header", default=None,
        help="Optional: read BOARD_FW/HW from board.h, patch input bin at --ver-patch-offset, "
             "and use for sw_version (default is read FW/HW from input bin only)",
    )
    ap.add_argument(
        "--ver-patch-offset", default="0x200",
        help="Offset in bin to write FW+HW version (2+2 bytes LE, default 0x200)",
    )
    ap.add_argument(
        "--rng-seed",
        default=None,
        help="Deterministic AES key/nonce from this seed (UTF-8 string, or hex:... for raw bytes). "
        "Reproducible output for testing; NOT for production security.",
    )
    ap.add_argument(
        "--fixed-exip-material",
        default=None,
        metavar="HEX",
        help="Fixed key+nonce hex: 48 hex digits per EXIP region (16-byte key + 8-byte nonce). "
        "Mutually exclusive with --rng-seed.",
    )
    ap.add_argument(
        "--ctr-iv-layout",
        choices=("nonce_high", "nonce_low"),
        default="nonce_high",
        help="How 8-byte EXIP ctr expands to AES-CTR IV: nonce_high=nonce||0^8 (default), "
        "nonce_low=0^8||nonce (try if ciphertext mismatches Windows tool with same key/nonce).",
    )
    ap.add_argument(
        "--use-hpm-tool",
        default=None,
        metavar="PATH",
        help="Path to hpm_img_util.exe (or via wine on Linux). "
        "When set, generate imageutil.json and delegate EXIP encryption to the official "
        "HPM tool (create_image command), producing output byte-identical to the GUI. "
        "Ignores --rng-seed, --fixed-exip-material, --ctr-iv-layout, --fw-length, "
        "--append-hpm-image-tail (the tool handles everything).",
    )
    args = ap.parse_args()

    if args.selftest:
        run_selftest()
        return

    fw_offset = _parse_int(args.fw_offset)
    hdr_offset = _parse_int(args.hdr_offset)
    load_addr = _parse_int(args.load_addr)
    entry = _parse_int(args.entry)
    mem_base = _parse_int(args.mem_base)
    exip_start = _parse_int(args.exip_start)
    exip_len = _parse_int(args.exip_len)

    if args.diff_bin is not None:
        diff_encrypted_bins(
            args.diff_bin[0],
            args.diff_bin[1],
            hdr_offset=hdr_offset,
            fw_offset=fw_offset,
            mem_base=mem_base,
            exip_start=exip_start,
            exip_len=exip_len,
        )
        return

    if not args.input or not args.output or not args.kek:
        sys.exit("ERROR: encrypt mode needs --input, --output, --kek (or use --diff-bin A B)")

    # --use-hpm-tool: delegate to hpm_img_util.exe create_image
    if args.use_hpm_tool is not None:
        _run_hpm_tool_encrypt(args)
        return

    if args.rng_seed is not None and args.fixed_exip_material is not None:
        sys.exit("ERROR: use only one of --rng-seed or --fixed-exip-material")

    kek = bytes.fromhex(args.kek)
    if len(kek) != 16:
        sys.exit("ERROR: KEK must be exactly 16 bytes (32 hex characters)")

    sw_version = _parse_int(args.sw_version)
    flash_opts = [_parse_int(x.strip()) for x in args.flash_opts.split(",")]
    if len(flash_opts) != 4:
        sys.exit("ERROR: --flash-opts must have exactly 4 values")

    with open(args.input, "rb") as f:
        raw_input = bytearray(f.read())

    print(f"[ENCRYPT] Input: {args.input} ({len(raw_input)} bytes)")

    ver_off = _parse_int(args.ver_patch_offset)
    if args.board_header:
        board_fw, board_hw = parse_board_versions(args.board_header)
        struct.pack_into("<HH", raw_input, ver_off, board_fw, board_hw)
        sw_version = board_fw
        print(
            f"[ENCRYPT] Patched from board.h @ 0x{ver_off:x}: FW={board_fw} HW={board_hw} "
            f"(sw_version={sw_version})"
        )
        with open(args.input, "wb") as f:
            f.write(raw_input)
        print(f"[ENCRYPT] Version written back to {args.input}")
    else:
        if len(raw_input) < ver_off + 4:
            sys.exit(
                "ERROR: input bin too small for version at offset 0x%x "
                "(need %d bytes; link .mg_pack_ver at 0x80003200 / file off 0x200)"
                % (ver_off, ver_off + 4)
            )
        fw_raw, hw_raw = struct.unpack_from("<HH", bytes(raw_input), ver_off)
        sw_version = fw_raw
        print(
            f"[ENCRYPT] Version from input @ 0x{ver_off:x}: FW={fw_raw} HW={hw_raw} "
            f"(sw_version={sw_version})"
        )

    fw_data = extract_raw_firmware(bytes(raw_input), fw_offset)
    if args.fw_length is not None:
        flen = _parse_int(args.fw_length)
        if flen <= 0:
            sys.exit("ERROR: --fw-length must be positive")
        if len(fw_data) < flen:
            sys.exit(
                "ERROR: --fw-length %d exceeds extracted firmware (%d bytes)"
                % (flen, len(fw_data))
            )
        if len(fw_data) > flen:
            print(
                f"[ENCRYPT] Truncate firmware: {len(fw_data)} -> {flen} bytes (--fw-length)"
            )
            fw_data = fw_data[:flen]
    print(f"[ENCRYPT] Firmware data: {len(fw_data)} bytes")

    boot_image = build_boot_image(
        fw_data, fw_offset, hdr_offset, load_addr, entry, sw_version, flash_opts
    )
    if args.append_hpm_image_tail:
        if args.fw_length is None:
            sys.exit(
                "ERROR: --append-hpm-image-tail requires --fw-length "
                "(tail is read from input after truncated firmware)"
            )
        tail_len = hdr_offset
        pos = fw_offset + len(fw_data)
        chunk = bytes(raw_input)[pos : pos + tail_len]
        if len(chunk) < tail_len:
            chunk = chunk + b"\x00" * (tail_len - len(chunk))
            print(
                f"[ENCRYPT] Input short for tail: padded {tail_len - len(chunk)} bytes "
                f"with 0x00 (need {tail_len} from 0x{pos:x})"
            )
        boot_image = boot_image + chunk
        pad16 = (-len(boot_image)) % 16
        if pad16:
            boot_image = boot_image + b"\x00" * pad16
        print(
            f"[ENCRYPT] Appended HPM tail: {tail_len} bytes from input[0x{pos:x}:...) "
            f"(= hdr_offset 0x{hdr_offset:x}); boot image -> {len(boot_image)} bytes"
        )
    print(f"[ENCRYPT] Boot image: {len(boot_image)} bytes")

    if args.fixed_exip_material is not None:
        rand_fn = make_fixed_material_randbytes(args.fixed_exip_material)
    elif args.rng_seed is not None:
        rand_fn = make_seeded_randbytes(_seed_to_bytes(args.rng_seed))
    else:
        rand_fn = None
        print(
            "[ENCRYPT] NOTE: EXIP key/nonce from os.urandom (this run only). "
            "HPM 镜像助手若未使用「随机」且界面 Key/Nonce 不变，则同一固件多次导出前 0x100 相同；"
            "要与助手一致请使用 ENCRYPT_FIXED_EXIP_MATERIAL / --fixed-exip-material（界面 32+16 hex 连成 48 hex）。"
        )

    encrypted = encrypt_exip_image(
        boot_image,
        kek,
        mem_base,
        [(exip_start, exip_len)],
        randbytes=rand_fn,
        ctr_iv_layout=args.ctr_iv_layout,
    )
    print(f"[ENCRYPT] Encrypted image: {len(encrypted)} bytes")

    with open(args.output, "wb") as f:
        f.write(encrypted)

    print(f"[ENCRYPT] Output: {args.output}")


if __name__ == "__main__":
    main()
