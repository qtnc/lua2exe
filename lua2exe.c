// This code is part of lua2exe
// Lua2exe compiler
#include<lua/lua.h>
#include<lua/lualib.h>
#include<lua/lauxlib.h>
#include<zlib.h>
#include<stdio.h>
#include "lua2exe.h"

#define streq(a,b) (0==strcmp(a,b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define DEFAULT_BASE_EXECUTABLE "lua.lvm"

#if defined __WIN32 || defined __WIN64
#define DEFAULT_OUTPUT_FILE "a.exe"
#else
#define DEFAULT_OUTPUT_FILE "a.out"
#endif

#define BOOL int
#define TRUE 1
#define FALSE 0

static BOOL quiet = FALSE;

static const char* doCompress (const char* in, int inlen, int* outlen) {
char* buf = malloc(inlen+8);
if (!buf) return NULL;
z_stream z;
memset(&z, 0, sizeof(z));
z.next_in = in;
z.avail_in = inlen;
z.next_out = buf;
z.avail_out = inlen+8;
if (Z_OK!=deflateInit(&z,9)) return NULL;
if (Z_STREAM_END!=deflate(&z,Z_FINISH)) return NULL;
if (outlen) *outlen = inlen +8 -z.avail_out;
return buf;
}

static int writechunk (lua_State* l, EmbeddedChunkInfo* i, FILE* out, const char* chunkname, BOOL dbg) {
int n=0, t=0, length=0, clength=0;
lua_pushboolean(l, !dbg);
lua_call(l, 2, 1);
const char* data = lua_tolstring(l, -1, &length);
const char*cdata = doCompress(data, length, &clength);
if (!cdata) return 1;
i->offset = ftell(out);
i->length = clength;
while (t<clength && (n=fwrite(cdata+t, 1, clength-t, out))>0) t+=n;
free(cdata);
if (t<clength) return 1;
lua_pop(l,1);
if (!quiet) printf("%s: %d => %d (%d%%)\r\n", chunkname, length, clength, 100 - (100 * clength / length));
return 0;
}

static int printVersion () {
printf("%s", 
"Lua2Exe version 1.0 (July 2012)\r\n"
"Copyright © 2012, QuentinC (http://quentinc.net/) \r\n"
"Using luajit 2.0.0.10 as lua VM (© 2005-2012, Mike Pall) (http://luajit.org/) \r\n"
); return 0;
}

int printHelp () {
printVersion();
printf("%s", 
"Usage: lua2exe options scripts... \r\n"
"Scripts: scripts to compile and embed into the lua executable. \r\n"
"Options: zero, one or more of the following options: \r\n"
"-f\tRead the list of files to embed from a file.\r\n"
"-i\tRead the list of files to embed from stdin.\r\n"
"-o [file] \tSet output file. Default to " DEFAULT_OUTPUT_FILE "\r\n"
"-b [file] \tSet base lua executable. Default to " DEFAULT_BASE_EXECUTABLE "\r\n"
"-g \tGenerate debug info, disabled by default.\r\n"
"-q\tBe quiet, don't output anything except on error.\r\n"
"-h \tPrint this help \r\n"
"-v \tPrint version information\r\n\r\n"
"First script given is considered as being the main one and is automatically run when the generated executable starts. \r\n"
"Other scripts are compiled and embedded, but are only run when called by require. \r\n"
"An embedded script has always the priority against a script file with the same name beside the executable. \r\n"
); return 0;
}

void preparename (char* s) {
char* c = strrchr(s,'.');
if (c) *c=0;
while (*(++s)) {
if (*s=='/' || *s=='\\') *s='.';
}}

int main (int argc, char** argv) {
lua_State* l = NULL;
char baseFile[300] = DEFAULT_BASE_EXECUTABLE;
char outputFile[300] = DEFAULT_OUTPUT_FILE;
FILE* inputFile = NULL;
int argi = 0;
BOOL keepDebugInfo = FALSE;

if (argc<2) return printHelp();
while (++argi<argc && argv[argi][0]=='-') {
if (streq(argv[argi], "-o")) strcpy(outputFile, argv[++argi]);
else if (streq(argv[argi], "-b")) strcpy(baseFile, argv[++argi]);
else if (streq(argv[argi], "-f")) inputFile = fopen(argv[++argi], "r");
else if (streq(argv[argi], "-i")) inputFile = (FILE*)stdin;
else if (streq(argv[argi], "-g")) keepDebugInfo = TRUE;
else if (streq(argv[argi], "-q")) quiet = TRUE;
else if (streq(argv[argi], "-h") || streq(argv[argi], "--help") || streq(argv[argi], "-?")) return printHelp();
else if (streq(argv[argi], "-v")) return printVersion();
else fprintf(stderr, "Warning: unknown option: %s\r\n", argv[argi]);
}
if (!quiet) printVersion();

int n = 0, t=0;
GlobalChunkInfo ginfo = { 0, 0, 0, MAGIC };
EmbeddedChunkInfo e;
int buflen = 4096, bufpos = 0;
char* buf = malloc(buflen);
if (!buf) return 1;

FILE* out = fopen(outputFile, "wb");
FILE* in = fopen(baseFile, "rb");
if (!out) { fprintf(stderr, "Couldn't open output file %s\r\n", outputFile); return 1; }
if (!in) { fprintf(stderr, "Couldn't read base file %s\r\n", baseFile); return 1; }
while ((n=fread(buf, 1, buflen, in))>0) fwrite(buf, 1, n, out);
fclose(in);

l = lua_open();
luaL_openlibs(l);
lua_getglobal(l, "string");
lua_getfield(l, -1, "dump");

unsigned long length=0; 
const char* data=NULL;

int output (char* fn) {
lua_pushvalue(l,-1);
if (luaL_loadfile(l, fn)) {
fprintf(stderr, "%s", lua_tostring(l,-1));
return 1;
}
ginfo.count++;
preparename(fn);
e.namelength = strlen(fn) +1;
if (writechunk(l, &e, out, fn, keepDebugInfo)) { fprintf(stderr, "Problem when writing to output file %s\r\n", outputFile); return 1; }
if (bufpos+e.namelength+sizeof(e)+8 >= buflen) buf = realloc(buf, buflen = MAX(buflen*3/2 +1, bufpos+e.namelength+sizeof(e)+16));
memcpy(buf + bufpos, &e, sizeof(e));
memcpy(buf + bufpos + sizeof(e), fn, e.namelength);
bufpos += e.namelength + sizeof(e);
return 0;
}

for (; argi<argc; argi++) {
if (output(argv[argi])) return 1;
}
if (inputFile) {
char line[512]={0};
while (!feof(inputFile) && !ferror(inputFile) && fgets(line,511,inputFile)) {
if (*line=='#' || *line==';' || *line=='\n') continue;
char* c = strchr(line,'\n');
if (c) *c=0;
c=line;
while(*c&&*c<=32) c++;
if (strlen(c)<=0) continue;
if (output(c)) return 1;
}
fclose(inputFile);
}

ginfo.offset = ftell(out);
ginfo.length = bufpos;
n=0; t=0;
while (t<bufpos && (n=fwrite(buf+t, 1, bufpos-t, out))>0) t+=n;
if (t<bufpos) { fprintf(stderr, "Problem when writing to output file %s\r\n", outputFile); return 1; }
if (1!=fwrite(&ginfo, sizeof(ginfo), 1, out)) { fprintf(stderr, "Problem when writing to output file %s\r\n", outputFile); return 1; }
fflush(out);
fclose(out);
return 0;
}
