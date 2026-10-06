# SaniGhost

`SaniGhost` is an Android and Python data sanitization engine designed for secure file wiping, SQLite database destruction, and directory shredding.

## Features

- **Secure File Shredding**: Overwrites target files with random bytes prior to deletion to prevent post-incident forensic recovery.
- **SQLite Sanitization**: Clears tables and executes `VACUUM` before shredding database files.
- **Buffer & Log Purging**: Recursively shreds temporary directories, image buffers, and log stores.

## Usage

```python
from sanighost import GhostSanitizer

sanitizer = GhostSanitizer(target_dirs=["logs", "cache"], db_path="app_data.db")
sanitizer.purge_evidence()
```
