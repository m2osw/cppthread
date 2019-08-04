
<p align="center">
<img alt="cppthread" title="C++ library to easily manage thread and thread pools including communication and mutexes."
src="https://raw.githubusercontent.com/m2osw/cppthread/master/doc/cppthread.png" width="191" height="248"/>
</p>

# Introduction

The cppthread started based on the functionality offered by the
`snap_thread.cpp/.h` from the libsnapwebsites.

It includes support for communication between threads, a thread pool,
mutexes, etc.


# Features

The library supports a thread controller and a runner by default. This
allows for a lot of safety that threads do not otherwise offer in C++
(i.e. it is too late to try to destroy a thread once you are in the
destructor because many of the virtual functions are going to be wrong
by then.)

* Thread Controller (`thread`)
* Runner, the actual thread (`runner`)
* Pool workers (`pool`, `worker`)
* Guard variables (`mutex`, `guard`)
* Control thread lifetime (`life`)
* Inter thread communication (`fifo`)


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snaplogger/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
