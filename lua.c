// This code is part of lua2exe
// Base executable VM
#include<lua/lua.h>
#include<lua/lualib.h>
#include<lua/lauxlib.h>
#include<zlib.h>
#include<stdio.h>
#include "lua2exe.h"

lua_State* l = NULL;
GlobalChunkInfo ginfo = {0, 0, 0};
char selfname[300] = {0};

#if defined __WIN32 || defined __WIN64
#include<windows.h>
static BOOL getSelfName (char* buf, int len) {
return GetModuleFileNameA(NULL, buf,len);
}
#else
#error Platform currently unsupported
#endif

const char* doDecompress (const char* in, int inlen, int* outlen) {
int buflen = inlen;
char* buf = malloc(buflen);
if (!buf) return NULL;
z_stream z;
memset(&z, 0, sizeof(z));
z.next_in = in;
z.avail_in = inlen;
z.next_out = buf;
z.avail_out = buflen;
if (Z_OK!=inflateInit(&z)) return NULL;
while (Z_OK==inflate(&z,0)) {
int newlen = buflen*3/2+1;
buf = realloc(buf, newlen);
z.next_out = buf + buflen;
z.avail_out = newlen -buflen;
buflen = newlen;
}
if (outlen) *outlen = buflen -z.avail_out;
return buf;
}


static int loadembedded (lua_State* l, const char* name, unsigned long offset, unsigned long length) {
char* buf = malloc(length);
if (!buf) return LUA_ERRMEM;
FILE* fp = fopen(selfname, "rb");
if (!fp) { lua_pushstring(l, "Unable to read into the executable file"); return -3; }
fseek(fp, offset, SEEK_SET);
int n=0, total=0;
while (total<length && (n=fread(buf+total, 1, length-total, fp))>0) total+=n;
fclose(fp);
if (total<length) { lua_pushstring(l, "Unable to read into the executable file"); return -3; }
char name2[1024];
sprintf(name2, "@%s", name);
int dlen=0;
const char* dbuf = doDecompress(buf, length, &dlen);
if (!dbuf) { lua_pushstring(l,"Error during data decompression"); return -4; }
int re = luaL_loadbuffer(l, dbuf, dlen, name2);
free(buf);
free(dbuf);
return re;
}

static int loadembed (lua_State* l) {
varnum vn;
const char* name = luaL_checkstring(l,1);
vn.d = luaL_checknumber(l,2);
if (loadembedded(l, name, vn.n1, vn.n2)) return lua_error(l);
else return 1;
}

int lua_getbacktrace (lua_State* l) {
lua_getglobal(l, "debug");
lua_getfield(l, -1, "traceback");
lua_pushvalue(l,1);
lua_pushinteger(l,2);
if (lua_pcall(l, 2, 1,0)) {
printf("Error in error handling !\r\n");
}
return 1;
}

int main (int argc, char** argv) {
l = lua_open();
if (!l) goto error;
luaL_openlibs(l);
getSelfName(selfname,300);
FILE* fp = fopen(selfname, "rb");
if (!fp) goto error;
fseek(fp, -sizeof(ginfo), SEEK_END);
if (fread(&ginfo, 1, sizeof(ginfo), fp) != sizeof(ginfo)) goto error;
if (ginfo.magic!=MAGIC || ginfo.count<1) goto error;
fseek(fp, ginfo.offset, SEEK_SET);
char* buf = malloc(ginfo.length);
if (!buf) goto error;
int n=0, m=0;
while (n<ginfo.length && (m=fread(buf+n, 1, ginfo.length-n, fp))>=0) n+=m;
fclose(fp);

char firstname[1024]={0};
unsigned long firstoffset=0, firstlength=0;
lua_newtable(l);
m=n=0; 
EmbeddedChunkInfo e;
for(n=0; n<ginfo.count; n++) {
e = *(EmbeddedChunkInfo*)(buf +m);
m += sizeof(e);
if (n==0) { strcpy(firstname, buf+m); firstoffset=e.offset; firstlength=e.length; }
lua_pushstring(l, buf+m);
m += e.namelength;
varnum vn;
vn.n1 = e.offset;
vn.n2 = e.length;
lua_pushnumber(l,vn.d);
lua_settable(l, -3);
}
free(buf);
lua_setglobal(l, "embeddedfiles");
lua_pushcclosure(l, loadembed, 0);
lua_setglobal(l, "loadembed");
{ lua_State* L = l;
#include "loader.c"
}

lua_settop(l,0);
lua_newtable(l);
for (n=0; n<argc; n++) {
lua_pushstring(l, argv[n]);
lua_rawseti(l, -2, n);
}
lua_setglobal(l, "arg");

lua_settop(l,0);
lua_pushcclosure(l, lua_getbacktrace, 0);
if (loadembedded(l, firstname, firstoffset, firstlength)) goto luaerror;
for (n=1; n<argc; n++) lua_pushstring(l, argv[n]);
if (lua_pcall(l, argc -1, LUA_MULTRET, -1 -argc)) goto luaerror;
if (lua_gettop(l)>=1 && lua_isnumber(l,-1)) return lua_tointeger(l,-1);
else return 0;

luaerror:
fprintf(stderr, "%s", lua_tostring(l,-1)); 
return 3;

error:
fprintf(stderr, "Unable to run this program; corrupted executable, permission problem or insufficient memory\r\n");
return 2;
}
