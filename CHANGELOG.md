# v0.0.2-alpha - Imports and Multi-Argument Functions

C compiler settings changed and some fixes ensued by compiler finds, plus expressions as fn parameters.
The biggest thing I personally like is syntax highlighting.

- Expressions now include minus operator
- Function parameters can now be int literals
- Started work in basic core library imports
- JavaScript buffered output system
- Simple Emacs mode made by slightly editting [SimpC-Mode](https://github.com/rexim/simpc-mode)
- Simple vim highlighting mostly vibe coded (shoutout [t3.chat](https://t3.chat))

# v0.0.1-alpha - Numbers And Fn Calls

First semi-working version with numbers and simple function calls

- Variable/Constant declaration with `:=` and `::` syntax respectively
- Function declaration with `fn` keyword
- Function calls with 0 or 1 argument get called ie `foo(); bar(a);` argument has to be an identifier for now
- Line comments with `//` are possible


