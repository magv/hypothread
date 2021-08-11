# Hypothread

Hypothread executes the command it was given as arguments, but
modifies its CPU affinity by selectively disabling logical
CPUs belonging to identical physical processor cores, so that
hyperthreading is avoided. If your program *really* performs
badly with hyperthreading, use hypothread to effectively disable
it.

Hypothread relies on `sched_setaffinity()` function and
`/proc/cpuinfo` file. Because of this it will only work on Linux
with GNU libc.

You can build hypothread with just:

    make

To use it on a given command run:

    hypothread command [argument] ...
