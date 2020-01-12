# `which(1)`

*not to be confused with GNU which`

It's a `which(1)` implementation written in C99. It's simple and does what you
expect it to. While `which` isn't a de-jure standard, it certainly is a
de-facto standard, even on the crustiest of Unices. This one supports common
extensions like `-a` and `-s`, plus it can be used as an ersatz `whereis(1)` in
a pinch.

Build system is Autotools. At least it's not CMake.
