flockwrap
=========

This is a wrapper library that adds advisory file locking capabilities to
programs that do not implement this type of locking by design. It works by
intercepting library calls to the open() family of functions and applying
an advisory lock on the resulting file descriptor i.e. by calling flock().

# Motivation

I wrote this library to safely create [Graphite](https://graphiteapp.org/)
backups. Graphite uses advisory locks to protect whisper databases from
concurrent access while they are being updated. Therefore, to safely create
backups of these databases (i.e. read a consistent snapshot), an advisory
lock must be placed when the backup is created as well. Unfortunately,
standard tools like `tar` or `rsync` have no such built-in capability.

Since the library is generic, it can be used in similar scenarios with any
files that are protected by advisory locks and with any program that opens
those files.

# How it works

If the file is opened in read-only mode, a shared lock (LOCK_SH) is applied.
Otherwise, an exclusive lock (LOCK_EX) is applied.

For locks to be applied, the FLOCKWRAP_PREFIX variable must be defined and set
to a path prefix that will be used to filter which files will be locked. The
prefix will be matched against the *canonical* path of the file (i.e. absolute
path starting at `/` with all symlinks resolved). To apply locks to any file,
set FLOCKWRAP_PREFIX to `/`.

# Building

```
make
```

# Usage

```
LD_PRELOAD=/path/to/flockwrap.so FLOCKWRAP_PREFIX=/ program [args]
```

To enable debugging (print to stderr which files are locked):

```
LD_PRELOAD=/path/to/flockwrap.so FLOCKWRAP_PREFIX=/ FLOCKWRAP_DEBUG=1 program [args]
```

# Compatibility

The code has been written and tested on Linux. It does include a few artefacts
that are Linux-specific, such as:
* Obtaining the full path that corresponds to a file descriptor by using
  `/proc/self/fd`.
* Using `dlsym()` with `RTLD_NEXT`.

Therefore, the code is *not* portable to other POSIX systems in its current
form. It can be ported, but this has not been a focus area, since the primary
use case is Linux.

# Credits

This library was inspired by [flockit](https://github.com/smerritt/flockit).
It does *not* use the same license because it was written from scratch and
therefore is not a derivative work.
