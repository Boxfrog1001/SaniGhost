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
/* Ghost ASCII Art: Neon Green -> Pure White -> Rich Purple */
"\033[38;2;0;255;100m⠀⠀⠀⠀⠀⠀⠀                      ⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\033[38;2;30;255;130m⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\033[38;2;70;255;160m⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣶⣶⣶⣶⣶⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\033[38;2;110;255;190m⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\033[38;2;150;255;215m⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣶⣶⣶⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\033[38;2;190;255;235m⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\033[38;2;225;255;245m⠀⠀⠀⠀⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣶⣶⡄⠀⠀⠀⠀\n"
"\033[38;2;245;255;250m⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀\n"
"\033[38;2;255;255;255m⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⠿⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠿⠿⠿⠿⠿⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀\n"
"\033[38;2;255;255;255m⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀\n"
"\033[38;2;255;255;255m⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⡿⠿⠀⠀⠀⠀⠀⠀⠀⠀⠸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀\n"
"\033[38;2;250;245;255m⠀⠀⠀⠀⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⡇⠀⠀⠀⠀\n"
"\033[38;2;240;230;255m⣀⣀⣀⣀⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣷⣶⣶⣶⡄⢸⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣷⣶⣶⣶⡆⢸⣿⣿⣿⣿⣧⣤⣤⣄⣀\n"
"\033[38;2;230;210;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣿⣿⠃⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;220;190;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;210;170;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⠿⠛⠛⠛⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⢿⣿⣿⡿⠛⠛⠛⠃⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;200;150;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;190;130;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;180;110;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;170;90;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣶⣶⣶⣶⣶⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣶⣶⣶⣶⣶⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;160;70;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;150;50;255m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;140;40;250m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;130;25;240m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;120;15;230m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;110;10;220m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;100;5;210m⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡏⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;90;0;200m⣿⣿⣿⣿⣿⣿⣿⣿⣿⡏⠀⠀⠸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠇⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿\n"
"\033[38;2;80;0;190m⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿\n"
"\033[38;2;70;0;180m⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠸⣿⣿⣿⣿⣿\n"
"\033[0m";

static void print_help(const char *prog_name) {
    printf("\033[38;2;0;255;128mSaniGhost\033[0m — \033[38;2;255;255;255mPermanently and securely erase files so they can never be recovered.\033[0m\n\n");
    printf("\033[1;32mUSAGE:\033[0m\n");
    printf("  %s [OPTIONS] [FILES / FOLDERS / DRAG & DROP...]\n\n", prog_name);
    printf("\033[1;32mDRAG & DROP / DIRECT TARGETS:\033[0m\n");
    printf("  %s secret.txt ./logs/ /media/usb/data/\n", prog_name);
    printf("  \033[90m(Just drag any file or folder from your desktop into this terminal!)\033[0m\n\n");
    printf("\033[1;32mWHAT TO ERASE (FLAGS):\033[0m\n");
    printf("  -f, --file <path>       Erase a specific file (can specify multiple)\n");
    printf("  -d, --dir <path>        Erase a whole folder and everything inside it\n");
    printf("  -b, --db <path>         Erase and clean an SQLite database file\n\n");
    printf("\033[38;2;200;160;255mSECURITY OPTIONS:\033[0m\n");
    printf("  -p, --passes <num>      How many times to overwrite the data (Default: 3)\n");
    printf("  -n, --dry-run           Test run only (shows what would happen without deleting)\n");
    printf("  -y, --yes               Skip confirmation prompt\n");
    printf("  -q, --quiet             Silent mode (don't print progress to the screen)\n");
    printf("  -i, --interactive       Launch interactive drag-and-drop prompt\n");
    printf("  -h, --help              Show this help menu\n\n");
    printf("\033[38;2;175;120;255mEXAMPLES:\033[0m\n");
    printf("  # Drag and drop any file directly:\n");
    printf("  %s my_keys.pem\n\n", prog_name);
    printf("  # Erase a folder with no confirmation prompt:\n");
    printf("  %s -d ./old_logs/ -y\n\n", prog_name);
    printf("  # Launch interactive drag-and-drop terminal session:\n");
    printf("  %s\n\n", prog_name);
}

static double get_time_sec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
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
        {"file",    required_argument, 0, 'f'},
        {"dir",     required_argument, 0, 'd'},
        {"db",      required_argument, 0, 'b'},
        {"passes",  required_argument, 0, 'p'},
        {"dry-run", no_argument,       0, 'n'},
        {"yes",     no_argument,       0, 'y'},
        {"quiet",       no_argument,       0, 'q'},
        {"interactive", no_argument,       0, 'i'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    bool interactive_mode = false;
    int opt;
    while ((opt = getopt_long(argc, argv, "f:d:b:p:nyqih", long_options, NULL)) != -1) {
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
            case 'i':
                interactive_mode = true;
                break;
            case 'h':
            default:
                puts(BANNER);
                print_help(argv[0]);
                return 0;
        }
    }

    /* Ingest any direct drag-and-drop positional arguments (e.g. ./sanighost file1.txt folder/) */
    for (int i = optind; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (dir_count < 256) dirs[dir_count++] = argv[i];
            } else {
                if (file_count < 256) files[file_count++] = argv[i];
            }
        } else {
            /* If not found yet, default to file target */
            if (file_count < 256) files[file_count++] = argv[i];
        }
    }

    /* If -i specified or no targets provided, launch interactive drag-and-drop prompt mode */
    if (interactive_mode || (file_count == 0 && dir_count == 0 && db_count == 0)) {
        puts(BANNER);
        printf("\033[38;2;0;255;128m[+] SaniGhost Drag & Drop Zone\033[0m\n");
        printf("\033[38;2;255;255;255m    Drag and drop any file or folder from your desktop into this terminal,\n");
        printf("    then press ENTER to securely erase it (or type 'q' to quit):\033[0m\n\n");
        printf("\033[38;2;200;160;255mDROP TARGET >> \033[0m");
        fflush(stdout);

        static char input_buf[PATH_MAX];
        if (!fgets(input_buf, sizeof(input_buf), stdin)) {
            return 0;
        }

        /* Strip trailing newline and potential GUI shell quotes */
        size_t len = strlen(input_buf);
        while (len > 0 && (input_buf[len - 1] == '\n' || input_buf[len - 1] == '\r' || input_buf[len - 1] == ' ')) {
            input_buf[--len] = '\0';
        }

        if (len == 0 || strcmp(input_buf, "q") == 0 || strcmp(input_buf, "exit") == 0) {
            printf("\033[38;2;175;120;255m[*] Exiting SaniGhost.\033[0m\n");
            return 0;
        }

        /* Handle single/double quotes added by terminal drag-and-drop */
        char *target_path = input_buf;
        if ((target_path[0] == '\'' || target_path[0] == '"') && target_path[len - 1] == target_path[0]) {
            target_path[len - 1] = '\0';
            target_path++;
        }

        struct stat st;
        if (stat(target_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            dirs[dir_count++] = target_path;
        } else {
            files[file_count++] = target_path;
        }
    } else if (config.verbose) {
        puts(BANNER);
    }

    if (!bypass_confirm && !config.dry_run) {
        printf("\033[38;2;255;70;70m[!] WARNING: This file will be forensically unretrievable. Are you sure? (y/N): \033[0m");
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
        printf("\033[38;2;0;255;128m[*] Initiating native POSIX forensic data sanitization...\033[0m\n");
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

    printf("\033[38;2;0;255;128m[+] Sanitization complete\033[0m in \033[38;2;255;255;255m%.3fs\033[0m. "
           "\033[38;2;200;160;255mShredded %lu files\033[0m (\033[38;2;255;255;255m%lu bytes overwritten\033[0m, %lu DBs). "
           "\033[38;2;175;120;255mErrors: %lu\033[0m\n",
           elapsed, (unsigned long)stats.files_shredded, (unsigned long)stats.bytes_overwritten, 
           (unsigned long)stats.dbs_sanitized, (unsigned long)stats.errors);

    return (stats.errors > 0) ? 1 : 0;
}
