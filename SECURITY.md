# Security Policy

## Supported Versions

SaniGhost maintains active security patches and forensic validation updates on the latest release versions.

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

We prioritize the security, forensic integrity, and privacy of users relying on SaniGhost.

If you discover a vulnerability, data remanence bug, or forensic leak:

1. **Do NOT open a public GitHub issue.**
2. Send an email to the security team or maintainer: **security@frogsec.org** (or utilize GitHub Private Vulnerability Reporting).
3. Include:
   - Detailed steps to reproduce the issue.
   - Operating system and kernel version (`uname -a`).
   - Underlying filesystem type (ext4, XFS, Btrfs, ZFS, NTFS).
   - Storage hardware type (NVMe SSD, SATA SSD, HDD).
   - Any raw sector dumps (`xxd`, `hexdump`, Autopsy timeline) demonstrating remanence.

### Response Timeline

- **Initial Response:** Within 48 hours acknowledging receipt of the disclosure.
- **Triage & Assessment:** Within 5 business days detailing reproducible status and remediation timeline.
- **Patch & Advisory:** Coordinated release and CVE assignment (if applicable) following patch validation.
