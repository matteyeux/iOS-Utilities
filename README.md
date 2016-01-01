#iOS Utilities

Here is some usefull iOS tools written by _Morpheus_ & J Levin. <br>

I added a Makefile to build these tools on Linux & OS X. <br>
Make sure you have GCC or Clang installed ! <br>
<br>

###Pzbx

Pbzx is an OTA Payload extractor written by _Morpheus_

```
User (/tmp/OTA/payloadv2)$ ls -l payload p.xz
-rw-r--r--  1 root      admin  443537788 Mar 18 13:47 p.xz
-rw-r--r--@ 1 user  admin  443539064 Feb 26 07:16 payload
User (/tmp/OTA/payloadv2)$ xz --decompress p.xz
User (/tmp/OTA/payloadv2)$ ls -l p
-rw-r--r--  1 root  admin  1310837211 Mar 18 13:47 p
User (/tmp/OTA/payloadv2)$ file p
p: VAX demand paged (first page unmapped) pure executable not stripped 
```

###OTAA

```
Usage: ota [-v] [-l] [-e file] _filename_
Where: -l: list files in update payload
       -e _file: extract file from update payload (use "*" for all files)
```

###Imagine

Imagine is an img3 file format dumper, with a focus on device tree files

```
Usage: ./imagine [-d] _filename_
```

Twitter : [@matteyeux](https://twitter.com/matteyeux)