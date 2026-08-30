// adapted from stdio.h
struct FILE {
    int _flags;
    *char _IO_read_ptr;
    *char _IO_read_end;
    *char _IO_read_base;
    *char _IO_write_base;
    *char _IO_write_ptr;
    *char _IO_write_end;
    *char _IO_buf_base;
    *char _IO_buf_end;

    *char _IO_save_base;
    *char _IO_backup_base;
    *char _IO_save_end;

    *_IO_marker _markers;

    *FILE _chain;

    int _fileno;
    int _flags2;
    long _old_offset;

    uint16 _cur_column;
    int8 _vtable_offset;
    [1]char _shortbuf;

    *_IO_lock_t _lock;
};

const int SEEK_SET = 0;
const int SEEK_CUR = 1;
const int SEEK_END = 2;

void   exit    (int status);

*void  malloc  (uint64 size);
void   free    (*void ptr);

int    puts    (*char string);
int    printf  (*char format, ...);
int    scanf   (*char format, ...);
int    putchar (int ch);
int    atoi    (*char str);

*FILE  fopen   (*char filename, *char modes);
int    fflush  (*FILE stream);
int    fclose  (*FILE stream);
uint64 fread   (*void ptr, uint64 size, uint64 n, *FILE stream);
uint64 fwrite  (*void ptr, uint64 size, uint64 n, *FILE stream);
int    fseek   (*FILE stream, long off, int whence);
long   ftell   (*FILE stream);
void   rewind  (*FILE stream);

*void  memcpy  (*void dest, *void src, uint64 n);
*char  strcpy  (*char dest, *char src);
uint64 strlen  (*char string);
