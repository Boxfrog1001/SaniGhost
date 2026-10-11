#define _POSIX_C_SOURCE 200809L

#include "shredder.h"
#include "sqlite_wipe.h"
#include "dir_walker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <limits.h>

static const char *BANNER =
/* SaniGhost Title: Bright Lime Green -> Pure White -> Deep Purple */
"\033[38;2;0;255;128m @@@@@@@                    @@@          @@@                        @@@     \n"
"\033[38;2;60;255;160m!@@                                     @@@                        @@@     \n"
"\033[38;2;130;255;195m!@!       @@@@@@@ @@@@@@@  @@@  @@@@@:  @@@@@@@   :@@@@@   :@@@@@  @@@@@@@ \n"
"\033[38;2;190;255;225m!!@@!!   @@@@@@@@ @@@@@@@@ @@@ @@@@@@@! @@@@@@@@ !@@@@@@@ !@@@@@@@ @@@@@@@@\n"
"\033[38;2;255;255;255m !!@!!!  @@!  @@@ @@!  @@@ @@@ @@@  !@@ @@@  @@@ @@!  @@@ :!@!::   @@@     \n"
"\033[38;2;245;235;255m     !:! !@!  @!@ !@!  @!@ @!@ @!@  !@@ @!@  !@! !@!  @!@    :!@!: @!@  !@!\n"
"\033[38;2;225;200;255m    !:!  @!@!@!@! !!@  !@! !@! !@!@!@!@ @@!  @!@ !!@!@!@! !!@!@!@! !@!@!@!@\n"
"\033[38;2;200;160;255m:::: ::  :!!@!@@! !:!  @:! ::!   :!@!@@ !@!  @!:  :!@!@:   :!@!@:   !@!@!!:\n"
"\033[38;2;175;120;255m:: : :    :  :! :  :   : :   : :!@. !@@ : :  :    :  ::    :  ::     !:  : \n"
"\033[38;2;150;80;255m                                :!@@!:!                                    \n"
"\033[38;2;125;40;240m                                  : : :                                    \n\n"
/* Ghost Skull Binary Art: Neon Green -> Pure White -> Rich Purple */
"\033[38;2;0;255;128m                       ...                                  \n"
"\033[38;2;21;255;138m                .010010101011     1101001010.               \n"
"\033[38;2;42;255;149m            01010010101010100    110001000101110010         \n"
"\033[38;2;63;255;159m          .0110101010101010100  .0010101010101010110.       \n"
"\033[38;2;85;255;170m        .101101010101010101010  001010101010101010110.      \n"
"\033[38;2;106;255;180m        1001001101010101010101  11010101110110010101101..   \n"
"\033[38;2;127;255;191m     .  .101001010101010101011  001010101010101010100  .0   \n"
"\033[38;2;148;255;202m         010110101010101010100  110101010101010101001       \n"
"\033[38;2;170;255;212m          00101101010110101001  01111001101100100001        \n"
"\033[38;2;191;255;223m                     011010101  110110          .           \n"
"\033[38;2;212;255;233m                       .00100    1100                       \n"
"\033[38;2;233;255;244m         01             11010.   00110            ..        \n"
"\033[38;2;255;255;255m        0011           .001011  001010.          .010.      \n"
"\033[38;2;243;234;252m       0010101010110100101011   1011010110101101011011.     \n"
"\033[38;2;232;214;249m       110101010101010101011       11010101000101010100     \n"
"\033[38;2;221;193;246m      110101010101010101011...    .00101010101010101011.    \n"
"\033[38;2;210;173;243m      001010101010101010100101.1010110110010101010101101    \n"
"\033[38;2;198;152;240m         0100101010100010101011.010110010101010100101.      \n"
"\033[38;2;187;132;237m        ...101001010101010101001101010101010110011010.      \n"
"\033[38;2;176;112;234m          ...010011101010101101001010101010100101..         \n"
"\033[38;2;165;91;231m          ....01010110001011010.1010101010110101            \n"
"\033[38;2;153;71;228m               0010101011010011.1000110101010010..          \n"
"\033[38;2;142;50;225m             .0 .. .101010010.  01. 0010010.  0.            \n"
"\033[38;2;131;30;222m                    .01010          1010.                   \n"
"\033[38;2;120;10;220m                        0.          .10                     \n"
"\033[0m";

static double get_time_sec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static void print_help(const char *prog_name) {
    printf("\033[38;2;0;255;128mSaniGhost — Native POSIX High-Performance Forensic Data Sanitizer\033[0m\n\n");
    printf("\033[38;2;255;255;255mUSAGE:\033[0m\n");
    printf("  %s [OPTIONS] [FILES / FOLDERS / DRAG & DROP...]\n\n", prog_name);
    printf("\033[38;2;200;160;255mQUICK ACTIONS:\033[0m\n");
    printf("  %s                      Launch Interactive Operations Dashboard\n", prog_name);
    printf("  %s <target>             Direct drag & drop file/folder shredding\n\n", prog_name);
    printf("\033[38;2;200;160;255mCLI FLAGS:\033[0m\n");
    printf("  -f, --file <path>       Erase a specific file\n");
    printf("  -d, --dir <path>        Erase a whole folder recursively\n");
    printf("  -b, --db <path>         Sanitize an SQLite database (.db, WAL, journal)\n");
    printf("  -p, --passes <num>      Overwrite pass count (Default: 3, Full Deep: 5+)\n");
    printf("  -n, --dry-run           Simulation mode (calculates slack space without writing)\n");
    printf("  -y, --yes               Bypass confirmation prompts\n");
    printf("  -q, --quiet             Silent mode\n");
    printf("  -h, --help              Show this help menu\n\n");
}

static void clean_path_input(char *buf) {
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' || buf[len - 1] == ' ')) {
        buf[--len] = '\0';
    }
    if (len >= 2 && ((buf[0] == '\'' && buf[len - 1] == '\'') || (buf[0] == '"' && buf[len - 1] == '"'))) {
        memmove(buf, buf + 1, len - 2);
        buf[len - 2] = '\0';
    }
}

int main(int argc, char *argv[]) {
    char *files[256];
    char *dirs[256];
    char *dbs[256];
    int file_count = 0, dir_count = 0, db_count = 0;

    ShredderConfig config = {
        .passes = 3,
        .dry_run = false,
        .verbose = true
    };
    bool bypass_confirm = false;

    static struct option long_options[] = {
        {"file",        required_argument, 0, 'f'},
        {"dir",         required_argument, 0, 'd'},
        {"db",          required_argument, 0, 'b'},
        {"passes",      required_argument, 0, 'p'},
        {"dry-run",     no_argument,       0, 'n'},
        {"yes",         no_argument,       0, 'y'},
        {"quiet",       no_argument,       0, 'q'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:d:b:p:nyqh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f':
                if (file_count < 256) files[file_count++] = optarg;
                break;
            case 'd':
                if (dir_count < 256) dirs[dir_count++] = optarg;
                break;
            case 'b':
                if (db_count < 256) dbs[db_count++] = optarg;
                break;
            case 'p':
                config.passes = atoi(optarg);
                if (config.passes < 1) config.passes = 1;
                break;
            case 'n':
                config.dry_run = true;
                break;
            case 'y':
                bypass_confirm = true;
                break;
            case 'q':
                config.verbose = false;
                break;
            case 'h':
            default:
                puts(BANNER);
                print_help(argv[0]);
                return 0;
        }
    }

    /* Ingest direct positional arguments (drag & drop execution) */
    for (int i = optind; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == 0 && S_ISDIR(st.st_mode)) {
            if (dir_count < 256) dirs[dir_count++] = argv[i];
        } else {
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    /* Interactive Operations Dashboard if launched with no arguments */
    static char target_buffer[PATH_MAX];
    if (file_count == 0 && dir_count == 0 && db_count == 0) {
        puts(BANNER);
        printf("\033[38;2;0;255;128m================================================================================\033[0m\n");
        printf("\033[38;2;255;255;255m                       SANIGHOST FORENSIC DASHBOARD                             \033[0m\n");
        printf("\033[38;2;0;255;128m================================================================================\033[0m\n");
        printf("\033[38;2;200;160;255m  [1] FULL FORENSIC PURGE (All 5-Stages: 5-Passes + Slack + FTL + Thumbnails)\033[0m\n");
        printf("\033[38;2;255;255;255m  [2] Quick Drag & Drop Zone (Drop file/folder directly)\033[0m\n");
        printf("\033[38;2;255;255;255m  [3] Erase Entire Folder Recursively\033[0m\n");
        printf("\033[38;2;255;255;255m  [4] Sanitize SQLite Database (.db, WAL, journal & VACUUM)\033[0m\n");
        printf("\033[38;2;255;255;255m  [5] Simulation Test (Dry-Run Mode - Zero Disk Writes)\033[0m\n");
        printf("\033[38;2;175;120;255m  [6] View Chain-of-Custody & Forensic Protection Specs\033[0m\n");
        printf("\033[38;2;255;80;80m  [0] Exit\033[0m\n");
        printf("\033[38;2;0;255;128m================================================================================\033[0m\n\n");
        printf("\033[38;2;0;255;128mSELECT OPTION [1-6, 0] >> \033[0m");
        fflush(stdout);

        char choice[32];
        if (!fgets(choice, sizeof(choice), stdin)) return 0;
        clean_path_input(choice);

        if (strcmp(choice, "0") == 0 || strcmp(choice, "q") == 0 || strcmp(choice, "exit") == 0) {
            printf("\033[38;2;175;120;255m[*] Exiting SaniGhost.\033[0m\n");
            return 0;
        }

        if (strcmp(choice, "6") == 0) {
            printf("\n\033[38;2;0;255;128m[+] SaniGhost 5-Stage Chain-of-Custody Pipeline:\033[0m\n");
            printf("  1. Synchronous Multi-Pass Overwrites (O_SYNC / fdatasync)\n");
            printf("  2. Hardware Controller Write-Barrier Flush\n");
            printf("  3. Pre-deletion 32-Character Hex Renaming (Dentry Table Sanitization)\n");
            printf("  4. Timestamp & Length Zeroing (utimensat to epoch {0,0}; ftruncate to 0)\n");
            printf("  5. Journal Flushing (parent dir fsync) & SSD Flash FTL Discard (FITRIM)\n");
            printf("  6. FreeDesktop Canonical URI MD5 Thumbnail Cache Purge\n\n");
            return 0;
        }

        if (strcmp(choice, "1") == 0) {
            /* Option 1: Full Forensice Purge */
            config.passes = 5; /* Deep 5-pass DoD/NIST cycle */
            printf("\n\033[38;2;0;255;128m[+] FULL FORENSIC PURGE ACTIVATED (5-Pass Synchronous Overwrites + FTL + Slack + Thumbnails)\033[0m\n");
            printf("\033[38;2;255;255;255mEnter target path to purge (or drag & drop here): \033[0m");
            fflush(stdout);
            if (!fgets(target_buffer, sizeof(target_buffer), stdin)) return 0;
            clean_path_input(target_buffer);
        } else if (strcmp(choice, "2") == 0) {
            printf("\n\033[38;2;0;255;128m[+] Drag & Drop Target (File or Directory): \033[0m");
            fflush(stdout);
            if (!fgets(target_buffer, sizeof(target_buffer), stdin)) return 0;
            clean_path_input(target_buffer);
        } else if (strcmp(choice, "3") == 0) {
            printf("\n\033[38;2;0;255;128m[+] Enter Directory Path to Erase Recursively: \033[0m");
            fflush(stdout);
            if (!fgets(target_buffer, sizeof(target_buffer), stdin)) return 0;
            clean_path_input(target_buffer);
        } else if (strcmp(choice, "4") == 0) {
            printf("\n\033[38;2;0;255;128m[+] Enter SQLite Database File (.db): \033[0m");
            fflush(stdout);
            if (!fgets(target_buffer, sizeof(target_buffer), stdin)) return 0;
            clean_path_input(target_buffer);
            dbs[db_count++] = target_buffer;
        } else if (strcmp(choice, "5") == 0) {
            config.dry_run = true;
            printf("\n\033[38;2;255;200;0m[!] DRY-RUN SIMULATION (Zero Disk Writes)\033[0m\n");
            printf("Enter target file or folder to test: ");
            fflush(stdout);
            if (!fgets(target_buffer, sizeof(target_buffer), stdin)) return 0;
            clean_path_input(target_buffer);
        } else {
            printf("\033[1;31m[-] Invalid option.\033[0m\n");
            return 1;
        }

        if (strlen(target_buffer) == 0) {
            printf("\033[1;31m[-] No target specified.\033[0m\n");
            return 1;
        }

        if (db_count == 0) {
            struct stat st;
            if (stat(target_buffer, &st) == 0 && S_ISDIR(st.st_mode)) {
                dirs[dir_count++] = target_buffer;
            } else {
                files[file_count++] = target_buffer;
            }
        }
    } else if (config.verbose) {
        puts(BANNER);
    }

    if (!bypass_confirm && !config.dry_run) {
        printf("\n\033[38;2;255;70;70m[!] WARNING: Target will be permanently and forensically unrecoverable.\033[0m\n");
        printf("\033[38;2;255;255;255m    Proceed with permanent destruction? (y/N): \033[0m");
        fflush(stdout);
        char answer[16];
        if (!fgets(answer, sizeof(answer), stdin) || (answer[0] != 'y' && answer[0] != 'Y')) {
            printf("\033[38;2;200;160;255m[*] Operation cancelled by user.\033[0m\n");
            return 0;
        }
    }

    SanitizeStats stats = {0};
    stats.start_time = get_time_sec();

    if (config.verbose) {
        printf("\n\033[38;2;0;255;128m[*] Initiating SaniGhost Forensic Chain-of-Custody Pipeline...\033[0m\n");
    }

    /* 1. Target files */
    for (int i = 0; i < file_count; i++) {
        shred_file(files[i], &config, &stats);
    }

    /* 2. Target SQLite databases */
    for (int i = 0; i < db_count; i++) {
        sanitize_sqlite_db(dbs[i], &config, &stats);
    }

    /* 3. Target directories */
    for (int i = 0; i < dir_count; i++) {
        shred_directory(dirs[i], &config, &stats);
    }

    stats.end_time = get_time_sec();
    double elapsed = stats.end_time - stats.start_time;

    printf("\n\033[38;2;0;255;128m[+] Sanitization complete\033[0m in \033[38;2;255;255;255m%.3fs\033[0m.\n"
           "\033[38;2;200;160;255m    • Files shredded: %lu\033[0m\n"
           "\033[38;2;200;160;255m    • Bytes overwritten: %lu bytes\033[0m\n"
           "\033[38;2;200;160;255m    • SQLite DBs sanitized: %lu\033[0m\n"
           "\033[38;2;175;120;255m    • Errors: %lu\033[0m\n",
           elapsed, (unsigned long)stats.files_shredded, (unsigned long)stats.bytes_overwritten, 
           (unsigned long)stats.dbs_sanitized, (unsigned long)stats.errors);

    return (stats.errors > 0) ? 1 : 0;
}
