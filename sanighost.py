import os
import shutil
import sqlite3

class GhostSanitizer:
    """
    Standard cleanup utility: Shreds databases, wipes image buffers, 
    and clears high-level application logs to ensure secure data destruction.
    """
    def __init__(self, target_dirs=None, db_path="recon_pivot.db"):
        if target_dirs is None:
            target_dirs = ["AK_Central/logs", "AK_Kinetic/buffers", "AK_Actuator/can_logs"]
        self.target_dirs = target_dirs
        self.db_path = db_path

    def shred_file(self, filepath):
        """Overwrites file with random bytes before deletion."""
        if os.path.exists(filepath):
            try:
                size = os.path.getsize(filepath)
                with open(filepath, "wb") as f:
                    f.write(os.urandom(size))
                os.remove(filepath)
                print(f"    [CLEAN] Shredded: {filepath}")
            except Exception as e:
                print(f"    [ERROR] Failed to shred {filepath}: {e}")

    def purge_evidence(self):
        print("[*] Sanitizer: Initiating forensic cleanup...")
        
        # 1. SQL Sanitization
        if os.path.exists(self.db_path):
            try:
                conn = sqlite3.connect(self.db_path)
                cursor = conn.cursor()
                cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
                tables = cursor.fetchall()
                for table in tables:
                    cursor.execute(f"DELETE FROM {table[0]}")
                conn.commit()
                conn.execute("VACUUM")
                conn.close()
                self.shred_file(self.db_path)
            except Exception as e:
                print(f"    [ERROR] Database wipe failed: {e}")

        # 2. Directory Shredding
        for directory in self.target_dirs:
            if os.path.exists(directory):
                for root, dirs, files in os.walk(directory):
                    for f in files:
                        self.shred_file(os.path.join(root, f))
                shutil.rmtree(directory)
        
        print("[+] Sanitizer: Environment neutralized. No local traces remain.")

if __name__ == "__main__":
    cleaner = GhostSanitizer()
    # cleaner.purge_evidence()
