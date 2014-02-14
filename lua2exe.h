// This code is part of lua2exe
// Lua2exe common header file
#ifndef LUA2EXE
#define LUA2EXE

// Lua2exe magic number = ascii "lua2exe\0"
#define MAGIC 0x6578653261756C00LL

typedef union {
struct { unsigned long n1, n2; };
double d;
} varnum;

typedef struct {
unsigned long offset;
unsigned long length :22;
unsigned long namelength :10;
} EmbeddedChunkInfo;

typedef struct {
unsigned long offset, length, count;
unsigned long long magic;
} GlobalChunkInfo;

#endif
