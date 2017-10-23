# Dependencies

* libtool
* autopoint
* libunistring-dev
* libsqlite3-dev
* libgcrypt20-dev
* libidn11-dev
* zlib1g-dev
* texinfo # TODO: Figure out how to tell the makefile not to generate documentation

# Generate GNUnet's documentation

```
$ cd gnunet/doc
$ texi2any --html --no-split --no-headers --force gnunet.texi
$ texi2any --html --no-split --no-headers --force gnunet-c-tutorial.texi
```
