Zip FUSE
========

This is an implementation of a zip-based file system which is based on libfuse and libzip.

How to run
==========

```
$ make
$ ./fusezip <zipfile> <mountpoint>
```

Dependencies
==================
1. libfuse
2. libzip

References
==========

https://github.com/libfuse/libfuse/blob/master/example/hello.c  
http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/html/  
https://www.cs.hmc.edu/~geoff/classes/hmc.cs135.201109/homework/fuse/fuse_doc.html  
