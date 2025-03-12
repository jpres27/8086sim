set compiler_flags=-MTd -nologo -EHa- -Gm- -GR- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4996 -FC -Zi
set linker_flags=-opt:ref -subsystem:console
pushd ..\build
cl %compiler_flags% -Fm8086asm.map ..\8086-Instruction-Decoder\8086asm.cpp -Fd /link -incremental:no -opt:ref %linker_flags%
popd