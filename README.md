
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

The basic  library supports a thread controller and a runner. This
allows for a lot of safety that threads do not otherwise offer in C++
(i.e. it is too late to try to destroy a thread once you are in the
destructor because most if not all of its virtual functions are going
to be wrong by then.)

The library also offers thread pools, guards/mutex, and inter-thread
communication with a FIFO implementation.

Here are the various objects found in the library:

* Thread Controller (`thread`)
* Runner, the actual thread (`runner`)
* Pool workers (`pool`, `worker`)
* Guard variables (`mutex`, `guard`)
* Control thread lifetime (`life`)
* Inter thread communication (`fifo`)
* Logging capability (`log`)
* Thread Specific Exceptions (`cppthread_exception`)
* Listing processes/threads
* Loading of plugins (.so files, `plugins`)
* Signals between plugins (`signal`)


# Safety

As mentioned above, many processes crash because of thread mishandling.
The main issue is to find yourself in the thread destructor when the thread
is still running. This is not likely to work as expected. If your class has
virtual functions, by the time the thread destructor is hit, your virtual
functions were removed from that object and thus your functions won't get
called. The default ones are called instead. So the results are likely
wrong (unless you implement your own thread class and it does it all for
you or you use callbacks instead of virtual functions).

This library tries to help you with that. You first want the `thread` objects
to be destroyed, those will force a `stop()` on the `runner` and then you
can safely destroy the `runner`. This means you want to create classes
where you first define a `runner` implementation and then the corresponding
thread:

    class my_class
    {
         ...

    private:
         my_runner::pointer_t           f_runner;
         cppthread::thread::pointer_t   f_thread;
    };

Such a class destructor first calls the `f_thread` destructor which calls
the `thread::stop()` function. That function asks the corresponding runner
(and we assume this is `f_runner`) to exit as soon as possible (the
`continue_running()` function now returns false). Once the runner returned,
the `thread::stop()` function detects that because of the join that
it does against the runner. It returns after the join completed which means
the runner is completely done (the actual corresponding kernel task did quit).

Once the `thread::stop()` returns, it is 100% safe to destroy the `f_runner`
object since the thread is not running anymore.

So beside the order in which you declare the thread and runner, the only
other rule is to test the `continue_running()` flag if your thread is
of the kind "running forever" and you have a very safe environment.

**WARNING:** If you want to create a class which derives from cppthread,
it can't own the runner. So a class like this is ill formed:

    class my_super_thread
        : cppthread::thread
    {
    ...
    private:
        cppthread::runner::pointer_t   f_my_runner;
    };

Your class is going to call the destructor of the runner before the one
from the thread. So again, that would destroy an object still in use (while
the thread is still running). This is exactly why we do not have the runner
defined in our cppthread::thread class.

# Inter-thread Messages

To share data between threads, you must have guards.

Whenever possible, when the thread is fed messages (rather than flags
changing in various variables), it is best to use the provided
`cppthread::fifo` object. This is a very fast queue which allows you to
safely send and receive messages (i.e. payloads). The FIFO automatically
uses a guard to add new messages and to remove existing messages. In other
words, the messages you get are 100% ready for you to access as you wish.

The name "FIFO" is somewhat misleading when using the thread pool system
and now that this class supports a priority system which allows for
processing messages _early_. The priority actually uses dependencies. So
message A can be marked as necessary to be able to process message B.
In that case, B will be sitting in the FIFO until the processing of A is
complete. When no such dependency exists, both messages can be processed
in parallel or at least in any order.

For threads that do not just receive/send messages, you must make sure
to use the `cppthread::guard` object with a `cppthread::mutex` to create
a barrier. Although a boolean flag may work as is without the need for
a mutex. However, with all the various CPU caches, it is very likely that,
once in while, a change to such a flag will not be visible for a while.


# Thread Pools

The library supports thread pools. A pool is an array of threads which
are expected to be used to execute work as it comes in. The next workload
gets taken on by the next available thread from that pool.

Our implementation specifically makes use of the `cppthread::fifo` which
indicates the next workload. Any one thread may wake up whenever a new
workload appears in the FIFO.

If you have more complex needs than a simple FIFO, then you would have
to implement your own thread pool. You can, of course, draw from our
implementation.

**WARNING:** Keep in mind that in this case each thread is just a shell
to run some task against a workload. The only dynamic variables you can
use are the ones found in the workload. The worker thread can only defined
static parameters (i.e. parameters you setup on startup and never change
later). This is because you can't know which thread is going to run next
and no order is guaranteed.


# Thread Lifetime

We have a `cppthread::life` object which can be used to stop a thread
at the time the `life` object is deleted. This can be useful because
the `cppthread::~thread()` function calls `stop()`, but it ignores all
the exceptions (this is because those exceptions would otherwise occur
in a destructor).

So this gives your software the ability to receive the `cppthread::runner`
exceptions as otherwise expected by this implementation.


# Exceptions

The crux of this library implementation, is the `runner::run()` function
which is where you want to have your code running in a separate thread.
The function is called in such a way that if an exception occurs, it
gets saved in the `thread` object and re-thrown in the owner of the
thread when joining with it (just after joining, that is).

Now there are issues with that scheme since at times you want to start
a thread for the entire duration of your application and thus you're
not going to be actively waiting on it to join. That means the exception
won't get propagated properly. For such cases, we at least have logs and
the `runner::leave()` function which can be used to detect those rogue
exceptions. You can, of course, have your own `try`/`catch` in your
implementation of the `runner::run()` function. But if those exceptions
are not expected and you have many threads, using the `runner::leave()`
function is going to be a lot less work for you.


# Logs

The library makes use of a very basic log mechanism which sends log
messages to `std::cerr` unless you define your own callback to capture
those messages (see `cppthread::set_log_callback()`).

If you are using the `snaplogger` library along the `cppthread` library,
then those logs are automatically going to be passed to the `snaplogger`
library (i.e. that library automatically calls the `set_log_callback()`
function for you).

Internally, the `cppthread` library takes care of locking the logger
as required so the logs from multiple threads do not get mangled.
If any error occurs while handling the mutexes, locking, unlocking,
then an error is printed in `std::cerr` and `std::terminate()`
gets called.


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/cppthread/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
