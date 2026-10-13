# SaniGhost v1.2

```text
                       ...                                  
                .010010101011     1101001010.               
            01010010101010100    110001000101110010         
          .0110101010101010100  .0010101010101010110.       
        .101101010101010101010  001010101010101010110.      
        1001001101010101010101  11010101110110010101101..   
     .  .101001010101010101011  001010101010101010100  .0   
         010110101010101010100  110101010101010101001       
          00101101010110101001  01111001101100100001        
                     011010101  110110          .           
                       .00100    1100                       
         01             11010.   00110            ..        
        0011           .001011  001010.          .010.      
       0010101010110100101011   1011010110101101011011.     
       110101010101010101011       11010101000101010100     
      110101010101010101011...    .00101010101010101011.    
      001010101010101010100101.1010110110010101010101101    
         0100101010100010101011.010110010101010100101.      
        ...101001010101010101001101010101010110011010.      
          ...010011101010101101001010101010100101..         
          ....01010110001011010.1010101010110101            
               0010101011010011.1000110101010010..          
             .0 .. .101010010.  01. 0010010.  0.            
                    .01010          1010.                   
                        0.          .10                     
```

<p align="center">
  <strong>Native POSIX High-Performance File, Database & Solid-State Sanitizer</strong><br>
  <em>Version 1.2: Multi-Level Hardware FTL Discard & Direct Kernel Page-Cache Bypass.</em>
</p>

---

## What's New in v1.2 (Official Release)

- **Kernel Page-Cache Bypass (`O_DIRECT`):** Overwrites bypass Linux RAM buffer caches completely via 4096-byte DMA sector alignment, preventing data from lingering in RAM for memory-dump carving tools (Volatility, LiME).
- **Solid-State Hardware FTL Discard:** Hooks `ioctl(FITRIM)` and allocation pressure sweeps to signal SSD/NVMe Flash Translation Layers (FTL) to immediately unmap and erase wear-leveled NAND blocks.
- **Cluster Slack Elimination:** Automatically rounds overwrite boundaries to the filesystem cluster size (`st.st_blksize`), destroying end-of-file slack fragments hunted by signature carvers.
- **Cryptographic Dentry Scrambler:** Mutates directory table entries into 32-character random cryptographic hex hashes prior to unlinking, scrubbing filenames from directory trees (`ext4_dir_entry_2`).
- **FreeDesktop Preview Hunter:** Computes canonical RFC 1321 MD5 hashes to purge cached image and document thumbnails across `~/.cache/thumbnails/` (`normal`, `large`, `x-large`, `xx-large`, `fail`).
- **SQLite Database Sanitizer:** Wipes database tables, purges Write-Ahead Logs (`.wal`), shared memory (`.shm`), and rollback journals, followed by `VACUUM;`.

---

## 8-Stage Chain of Custody Pipeline

Unlike conventional utilities that merely invoke standard `unlink()` or single-pass overwrites, SaniGhost v1.2 executes an exhaustive **8-Stage Forensic Chain of Custody Pipeline**:

```text
[ Target Payload ]
        │
        ├──► Level 1: POSIX Synchronous Multi-Pass Overwrite
        │       └─ 0x00 ➔ 0xFF ➔ /dev/urandom with explicit fdatasync()
        │
        ├──► Level 2: End-of-File Cluster Slack Zeroing
        │       └─ Padded to physical block boundaries (st.st_blksize)
        │
        ├──► Level 3: Kernel Page-Cache Bypass (O_DIRECT)
        │       └─ Sector-aligned DMA buffer writing directly to physical storage
        │
        ├──► Level 4: Directory Table / Dentry Cryptographic Scrubbing
        │       └─ Scrambles directory entry into 32-char hex string prior to unlink
        │
        ├──► Level 5: Timestamp & File Attribute Epoch Zeroing
        │       └─ utimensat() zeroes atime/mtime to {0,0} (1970); ftruncate() zeroes length
        │
        ├──► Level 6: FreeDesktop Thumbnail Cache Purge
        │       └─ MD5 URI hash identification deletes preview leaks in ~/.cache/thumbnails/
        │
        ├──► Level 7: Filesystem Journal Synchronization
        │       └─ fsync() on parent directory descriptor (O_DIRECTORY) commits deletion
        │
        └──► Level 8: Hardware Solid-State FTL Discard
                └─ ioctl(FITRIM) instructs SSD Flash Translation Layer to unmap NAND cells
```

---

## Installation & Compilation

### Requirements
- GCC or Clang (C99 standard)
- Make
- `libsqlite3-dev` (for SQLite database module)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/Boxfrog1001/SaniGhost.git
cd SaniGhost

# Compile optimized release binary
make

# Optional: Install globally to system PATH
sudo cp sanighost /usr/local/bin/
```

---

## Usage

```text
SaniGhost v1.2 — Native POSIX High-Performance Forensic Data Sanitizer
Official Multi-Level Hardware FTL & O_DIRECT Kernel Bypass Engine

USAGE:
  sanighost [OPTIONS] [FILES / FOLDERS / DRAG & DROP...]

QUICK ACTIONS:
  sanighost                      Launch Interactive Operations Dashboard
  sanighost <target>             Direct drag & drop file/folder shredding

CLI FLAGS:
  -f, --file <path>       Erase a specific file (can specify multiple)
  -d, --dir <path>        Erase a whole folder recursively
  -b, --db <path>         Sanitize an SQLite database (.db, WAL, journal)
  -p, --passes <num>      Overwrite pass count (Default: 3, Full Deep: 5+)
  -n, --dry-run           Simulation mode (calculates slack space without writing)
  -y, --yes               Bypass confirmation prompts
  -q, --quiet             Silent mode
  -v, --version           Print SaniGhost engine release version
  -h, --help              Show this help menu
```

### Examples

```bash
# Shred a single file with default 3-pass O_DIRECT pipeline
sanighost -f sensitive_plan.pdf -y

# 5-pass deep shred on an entire directory recursively
sanighost -d ./evidence_folder/ -p 5 -y

# Sanitize an SQLite database, including WAL, SHM, and VACUUM
sanighost -b app_data.db -y

# Test run without touching disk (calculates slack space padding)
sanighost -n file.docx
```

---

## License

Released under the **GPL-3.0 License**. Developed by [Boxfrog](https://github.com/Boxfrog1001) / FrogSec Labs.
