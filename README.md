![pcse](https://raw.githubusercontent.com/virchau13/pcse/master/pcse_logo.svg)
# pcse
pcse is a tree-walk interpreter for the the programming language known as "pseudocode" in the IGCSE Computer Science 0478 course, written in C++17.

## Why did you make this?
At the school I go to, the students are taught some Java before they are taught pseudocode so they get the fundamentals of programming down.

I noticed that many of my fellow students disliked pseudocode, but liked Java. So obviously, I wanted to know why.

I found out the reason they liked Java was because they could actually _run it_ and _see the impact of the code they wrote_; whereas pseudocode is simply something to write down, that doesn't actually do anything. After I realised that, I realised that an interpreter for the language could really help future students.

I've always wanted to write a programming language, so I decided to try it.

## Getting Started
In the future, there will be a VSCode extension to make installing easy. Since pcse is still in development, it must be built from scratch.  
Note that these instructions are written so that (hopefully) a non-programmer can follow them. If you have trouble understanding anything, please open an issue!

### Prerequisites
Install [git](https://git-scm.com/) and a C++ compiler. (Note that the compiler must support C++17.) Then, install [CMake](https://cmake.org/).  
(Optional) If you want to run the tests, install [Catch2](https://github.com/catchorg/Catch2/). If you don't, then comment out lines 13-14 of `CMakeLists.txt`.

### Building
Open a terminal, and clone this repo:
```
git clone https://github.com/virchau13/pcse
```
Then run these commands:
```
cd pcse
cmake .
make
```
After the commands have finished, `pcse` will be built.
In order to use it, you can write your code in a file in that directory, and then run `./pcse <filename>` in a terminal.

### Documentation and Examples

Documentation can be found in `docs/`, and examples can be found in `examples`. You can run a specific example by doing `./pcse examples/filename.pcse`.

## What is there left to do?
- [x] lexer
- [x] parser
- [ ] interpreter
- [ ] CLI
- [ ] VSCode extension
