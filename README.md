# iOS Utilities
Here is some usefull iOS tools written by J Levin. <br>
temp PS : Makefile might be broken right now 

I added a Makefile to build these tools on Linux & OS X. <br>
Make sure you have GCC or Clang installed ! <br>
<br>

###  Pzbx

Pbzx is an OTA Payload extractor written by _Morpheus_

```
$ ./pbzx  < payload > pb.xz  
Flags: 0x1000000
Chunk #1 (flags: 1000000, length: 6066008 bytes) 
Chunk #2 (flags: 1000000, length: 14240912 bytes) 
Chunk #3 (flags: 1000000, length: 14231772 bytes) 
Chunk #4 (flags: 1000000, length: 16777216 bytes) 
Warning: Can't find XZ header. Instead have 0x16c3201e(?).. This is likely not XZ data.
Chunk #5 (flags: 1000000, length: 16777216 bytes) 
Warning: Can't find XZ header. Instead have 0xf52ffff9(?).. This is likely not XZ data.
Chunk #6 (flags: 1000000, length: 14112620 bytes) 
Chunk #7 (flags: 1000000, length: 3866128 bytes) 
... # Business as usual in the modified version
Chunk #114 (flags: 1000000, length: 6347484 bytes) 
Chunk #115 (flags: 1000000, length: 3935192 bytes) 
Chunk #116 (flags: e7cfca, length: 2839680 bytes)
```

### OTAA

```
Usage: ota [-v] [-l] [-e file] _filename_
Where: -l: list files in update payload
       -e _file: extract file from update payload (use "*" for all files)
```

### Imagine

Imagine is an img3 file format dumper, with a focus on device tree files

```
Usage: ./imagine [-d] _filename_
```
