# SaniGhost v1.0.0 Release Notes

**Release Date:** October 2026  
**Repository:** [https://github.com/Boxfrog1001/SaniGhost](https://github.com/Boxfrog1001/SaniGhost)  
**License:** GNU General Public License v3.0 (GPLv3)

---

## 🔒 Security & Integrity Verification

To verify that release artifacts have not been tampered with or modified in transit, calculate their cryptographic digests and match them against the tables below:

### SHA-256 Checksums (`SHA256SUMS`)
```text
6564de81ffacf66a16cda8f7adffe7700df0ca32ed80302f943078c4aa22318a  sanighost-v1.0.0-linux-x86_64.tar.gz
8176e6a4ee096229271f46c8b803e7b14f9e91d96668e15bd891c39200fd3768  sanighost-v1.0.0-src.tar.gz
```

### Verification Command
```bash
# Verify SHA-256 integrity:
sha256sum -c SHA256SUMS
```

---

## 📦 Packaged Assets

| Asset File | Architecture | Description |
| :--- | :--- | :--- |
| `sanighost-v1.0.0-linux-x86_64.tar.gz` | Linux x86_64 | Pre-compiled standalone native binary + docs & license |
| `sanighost-v1.0.0-src.tar.gz` | Platform-independent | Full C99 source tree, Makefiles, tests, and headers |
| `SHA256SUMS` | Text | SHA-256 checksum manifest |
| `SHA512SUMS` | Text | SHA-512 checksum manifest |

---

## 🚀 What's New in v1.0.0

- **5-Stage Synchronous Overwrite Engine:**
  - Enforces `O_SYNC`, `fdatasync()`, and parent directory `fsync()` (forces journal flushes on ext4/XFS).
  - Multi-pass overwrite pattern: `0x00` -> `0xFF` -> cryptographically randomized stream (`/dev/urandom`).
- **Cluster Slack Space Zeroing:**
  - Dynamically calculates the physical block boundary (`st.st_blksize`) and wipes residual file slack space, destroying fragments targeted by signature carvers.
- **Hardware-Level Solid-State Flash Discard:**
  - Interfaces directly with `ioctl(FITRIM)` to command SSD/NVMe Flash Translation Layers (FTL) to deallocate physical NAND flash blocks.
- **FreeDesktop Automated Thumbnail Hunter:**
  - Computes RFC 1321 MD5 hashes of canonical file URIs and scrubs cached image/document previews across `~/.cache/thumbnails/` (`normal`, `large`, `x-large`, `xx-large`, `fail`).
- **SQLite Database Sanitizer:**
  - Wipes application tables, purges Write-Ahead Logs (`.wal`), shared memory (`.shm`), and rollback journals, followed by `VACUUM;`.
- **Drag & Drop Interactive Terminal:**
  - Native ANSI TrueColor green -> white -> purple visual gradient with direct drop-zone path parsing.
- **Audited for Memory Safety:**
  - Verified under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) for 0 leaks, 0 overflows, and clean heap reclamation.
