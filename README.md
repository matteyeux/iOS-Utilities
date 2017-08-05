# iOS Utilities

An assortment of various iOS-related tools written by [Jonathan Levin](https://twitter.com/Morpheus______).

For your convenience, a `Makefile` has been added to make the build process easier on macOS and Linux.

#### **WARNING: Do not use these tools if you are unfamiliar with the command line.**

### pbzx [(source)](http://newosxbook.com/src.jl?tree=listings&file=pbzx.c)

```
$ ./pbzx  < payload > payload2.xz
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

### ota [(source)](http://newosxbook.com/src.jl?tree=listings&file=ota.c)

```
Usage: ota [-v] [-l] [-e file] _filename_
Where: -l: list files in update payload
       -e _file: extract file from update payload (use "*" for all files)
```

### imagine [(source)](http://newosxbook.com/src.jl?tree=listings&file=6-bonus.c)

```
Usage: ./imagine [-d] _filename_
```
