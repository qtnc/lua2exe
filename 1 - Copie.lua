local ffi = require('ffi')
ffi.cdef[[
int __stdcall MessageBoxA (void* hwnd, const char* msg, const char* title, int options) ;
]]
ffi.C.MessageBoxA(nil, 'Niark niark niark !', 'Hello world !', 48)
