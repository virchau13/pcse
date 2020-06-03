# pcse
pcse is a tree-walk interpreter for the the programming language known as "pseudocode" in the IGCSE Computer Science 0478 course. (Yes, it does indeed have an official specification.)

### why did you make this?
At the school I go to, the students are taught some Java before they are taught pseudocode so they get the fundamentals of programming down.

I noticed that many of my fellow students disliked pseudocode, but liked Java. So obviously, I wanted to know why.

I found out the reason they liked Java was because they could actually _run it_ and _see the impact of the code they wrote_; whereas pseudocode is simply something to write down, that doesn't actually do anything. After I realised that, I realised that an interpreter for the language could really help future students.

I've always wanted to write a programming language, so I decided to try it. And I don't even have to write a specification! :D

### how do I use the interpreter?
This project has to be compiled with C++17.

To run the tests, install [Catch2](https://github.com/catchorg/Catch2/) and compile `test.cpp` (with `g++ -std=c++17 test.cpp`).

To get an interpreter, compile `main.cpp` (with `g++ -std=c++17 -O2 main.cpp -o pcse`) and run `pcse <filename>`. You can also simply run `pcse` to get a REPL.

### what is there left to do? (i.e. what compiles?)

- [x] lexer
- [ ] parser
- [ ] interpreter
- [ ] CLI
