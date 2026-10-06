import os
import shutil
import sqlite3
import time
import sys

class GhostSanitizer:
    """
    SaniGhost Core Engine: Multi-pass secure file shredder, SQLite table sanitizer,
    and directory wiper with dry-run support, permission verification, and progress reporting.
    
    100% Local Execution — Zero remote/cloud/Firebase dependencies.
    """

    def __init__(
        self,
        target_dirs=None,
        db_paths=None,
        target_files=None,
        passes=3,
        dry_run=False,
        progress_callback=None,
        log_level="INFO"
    ):
        """
        :param target_dirs: List of directory paths to recursively shred.
        :param db_paths: List of SQLite database file paths to sanitize.
        :param target_files: List of individual file paths to shred.
        :param passes: Number of overwrite passes (1=Random, 3=DoD 5220.22-M style, 7=Gutmann subset).
        :param dry_run: If True, simulates operations without writing or deleting.
        :param progress_callback: Optional function(status, file, bytes_processed) for UI/CLI integration.
        :param log_level: Logging verbosity ("QUIET", "INFO", "DEBUG").
        """
        self.target_dirs = target_dirs or []
        self.db_paths = db_paths or []
        self.target_files = target_files or []
        self.passes = max(1, passes)
        self.dry_run = dry_run
        self.progress_callback = progress_callback
        self.log_level = log_level

        # Statistics
        self.stats = {
            "files_shredded": 0,
            "bytes_overwritten": 0,
            "dbs_sanitized": 0,
            "errors": 0,
            "start_time": 0.0,
            "end_time": 0.0
        }

    def _log(self, message, level="INFO"):
        if self.log_level == "QUIET" and level != "ERROR":
            return
        prefix = f"[{'DRY-RUN' if self.dry_run else level}]"
        print(f"{prefix} {message}")

    def _report_progress(self, status, filepath="", bytes_processed=0):
        if self.progress_callback:
            try:
                self.progress_callback(status, filepath, bytes_processed)
            except Exception as e:
                self._log(f"Callback error: {e}", level="ERROR")

    def check_permissions(self, path):
        """Verifies read, write, and access permissions for target path."""
        if not os.path.exists(path):
            return False, "Path does not exist"
        if not os.access(path, os.R_OK):
            return False, "Read permission denied"
        if not self.dry_run and not os.access(path, os.W_OK):
            return False, "Write permission denied"
        return True, "OK"

    def shred_file(self, filepath):
        """
        Overwrites file with multi-pass patterns (0x00, 0xFF, random bytes)
        and executes os.fsync to bypass disk write caches before unlinking.
        
        Note on SSDs: Wear-leveling blocks may retain physical copies. Multi-pass + fsync
        mitigates OS/cache level forensics, but hardware-level TRIM is recommended for flash media.
        """
        ok, msg = self.check_permissions(filepath)
        if not ok:
            self._log(f"Skipping {filepath}: {msg}", level="ERROR")
            self.stats["errors"] += 1
            return False

        try:
            size = os.path.getsize(filepath)

            if self.dry_run:
                self._log(f"Would shred ({size} bytes, {self.passes} passes): {filepath}")
                self.stats["files_shredded"] += 1
                self.stats["bytes_overwritten"] += size * self.passes
                self._report_progress("WOULD_SHRED", filepath, size)
                return True

            if size > 0:
                with open(filepath, "r+b") as f:
                    fd = f.fileno()

                    for pass_num in range(1, self.passes + 1):
                        f.seek(0)

                        if pass_num == 1:
                            pattern = b"\x00" * min(size, 65536)
                        elif pass_num == 2 and self.passes >= 3:
                            pattern = b"\xFF" * min(size, 65536)
                        else:
                            pattern = os.urandom(min(size, 65536))

                        bytes_written = 0
                        while bytes_written < size:
                            chunk = pattern[:min(len(pattern), size - bytes_written)]
                            f.write(chunk)
                            bytes_written += len(chunk)

                        f.flush()
                        os.fsync(fd)
                        self.stats["bytes_overwritten"] += size

            os.remove(filepath)
            self.stats["files_shredded"] += 1
            self._log(f"Shredded ({size} B, {self.passes} pass): {filepath}")
            self._report_progress("SHREDDED", filepath, size)
            return True

        except Exception as e:
            self._log(f"Failed to shred {filepath}: {e}", level="ERROR")
            self.stats["errors"] += 1
            return False

    def sanitize_database(self, db_path):
        """
        Sanitizes SQLite database by deleting all rows, executing VACUUM
        to clear free-list pages, and then multi-pass shredding the file.
        """
        if not os.path.exists(db_path):
            return False

        self._log(f"Sanitizing SQLite DB: {db_path}")

        if self.dry_run:
            self._log(f"Would clear tables and shred DB: {db_path}")
            self.stats["dbs_sanitized"] += 1
            return True

        try:
            conn = sqlite3.connect(db_path)
            cursor = conn.cursor()
            cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';")
            tables = cursor.fetchall()

            for table in tables:
                cursor.execute(f'DELETE FROM "{table[0]}"')

            conn.commit()
            conn.execute("VACUUM")
            conn.close()
            self.stats["dbs_sanitized"] += 1
        except Exception as e:
            self._log(f"SQLite table wipe warning on {db_path}: {e}", level="ERROR")

        return self.shred_file(db_path)

    def purge_evidence(self, confirm=False):
        """
        Executes complete multi-target sanitization run.
        :param confirm: If True, prompts for CLI user confirmation before execution.
        """
        if confirm and not self.dry_run:
            ans = input("[!] WARNING: Confirm permanent data destruction? (y/N): ")
            if ans.lower() != "y":
                self._log("Operation cancelled by user.")
                return self.stats

        self.stats["start_time"] = time.time()
        self._log("Initiating forensic data sanitization...")

        # 1. Individual Target Files
        for filepath in self.target_files:
            if os.path.isfile(filepath):
                self.shred_file(filepath)

        # 2. SQLite Databases
        for db_path in self.db_paths:
            if os.path.isfile(db_path):
                self.sanitize_database(db_path)

        # 3. Directory Trees
        for directory in self.target_dirs:
            if os.path.exists(directory):
                for root, dirs, files in os.walk(directory):
                    for f in files:
                        self.shred_file(os.path.join(root, f))
                if not self.dry_run:
                    try:
                        shutil.rmtree(directory)
                        self._log(f"Removed empty directory tree: {directory}")
                    except Exception as e:
                        self._log(f"Directory remove error {directory}: {e}", level="ERROR")

        self.stats["end_time"] = time.time()
        elapsed = round(self.stats["end_time"] - self.stats["start_time"], 3)
        self._log(f"Sanitization complete in {elapsed}s. Shredded {self.stats['files_shredded']} files ({self.stats['bytes_overwritten']} bytes overwritten). Errors: {self.stats['errors']}")
        return self.stats

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="SaniGhost - Secure Multi-Pass File & DB Sanitizer")
    parser.add_argument("--dirs", nargs="*", help="Directories to shred recursively")
    parser.add_argument("--dbs", nargs="*", help="SQLite databases to sanitize and shred")
    parser.add_argument("--files", nargs="*", help="Specific files to shred")
    parser.add_argument("--passes", type=int, default=3, help="Number of overwrite passes (default: 3)")
    parser.add_argument("--dry-run", action="store_true", help="Simulate wipe without deleting data")
    parser.add_argument("--yes", action="store_true", help="Bypass confirmation prompt")

    args = parser.parse_args()

    cleaner = GhostSanitizer(
        target_dirs=args.dirs,
        db_paths=args.dbs,
        target_files=args.files,
        passes=args.passes,
        dry_run=args.dry_run
    )

    cleaner.purge_evidence(confirm=not args.yes and not args.dry_run)
