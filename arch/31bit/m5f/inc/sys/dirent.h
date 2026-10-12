/* sys/dirent.h for M5f programs on CMS: cmsrt.c's opendir/readdir list
   the files of one accessed disk ("x" = file mode, "." = A) */
#ifndef M5F_SYS_DIRENT_H
#define M5F_SYS_DIRENT_H
struct dirent { unsigned int d_ino; unsigned char d_type; char d_name[20]; };
typedef struct m8_dir DIR;
#define DT_UNKNOWN 0
#define DT_REG 8
#define DT_DIR 4
#define DT_FIFO 1
#define DT_CHR 2
#define DT_BLK 6
#define DT_LNK 10
#define DT_SOCK 12
#endif
