(module
 (import "env" "memory" (memory $0 256 256))
 (data (i32.const 1024) "(void)<::>{ out(\"no args works\"); }\00(void)<::>{ out(\"no args returning int\"); return 12; }\00(void)<::>{ out(\"no args returning double\"); return 12.25; }\00(int x)<::>{ out(\"  takes ints: \" + x);}\00(double d)<::>{ out(\"  takes doubles: \" + d);}\00(char* str)<::>{ out(\"  takes strings: \" + UTF8ToString(str)); return 7.75; }\00(int x, int y)<::>{ out(\"  takes multiple ints: \" + x + \", \" + y); return 6; }\00(int x, const char* str, double d)<::>{ out(\"  mixed arg types: \" + x + \", \" + UTF8ToString(str) + \", \" + d); return 8.125; }\00(int unused)<::>{ out(\"  ignores unused args\"); return 5.5; }\00(int x, int y)<::>{ out(\"  skips unused args: \" + y); return 6; }\00(double x, double y, double z)<::>{ out(\"  \" + x + \" + \" + z); return x + z; }\00(void)<::>{ out(\"  can use <::> separator in user code\"); return 15; }\00(void)<::>{ var x, y; x = {}; y = 3; x[y] = [1, 2, 3]; out(\"  can have commas in user code: \" + x[y]); return x[y][1]; }\00(void)<::>{ var jsString = \'\e3\81\93\e3\82\93\e3\81\ab\e3\81\a1\e3\81\af\'; var lengthBytes = lengthBytesUTF8(jsString); var stringOnWasmHeap = _malloc(lengthBytes); stringToUTF8(jsString, stringOnWasmHeap, lengthBytes+1); return stringOnWasmHeap; }\00(void)<::>{ var jsString = \'hello from js\'; var lengthBytes = jsString.length+1; var stringOnWasmHeap = _malloc(lengthBytes); stringToUTF8(jsString, stringOnWasmHeap, lengthBytes+1); return stringOnWasmHeap; }\00BEGIN\n\00    noarg_int returned: %d\n\00    noarg_double returned: %f\n\00    stringarg returned: %f\n\00string arg\00    multi_intarg returned: %d\n\00    multi_mixedarg returned: %f\n\00hello\00    unused_args returned: %d\n\00    skip_args returned: %f\n\00    add_outer returned: %f\n\00    user_separator returned: %d\n\00    user_comma returned: %d\n\00    return_str returned: %s\n\00    return_utf8_str returned: %s\n\00END\n\00\00\cc\1a\00\00\00\00\00\00\00\00\00\00\00\00\00\00T!\"\19\0d\01\02\03\11K\1c\0c\10\04\0b\1d\12\1e\'hnopqb \05\06\0f\13\14\15\1a\08\16\07($\17