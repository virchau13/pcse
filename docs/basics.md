# pcse pseudocode

`pcse` follows the styleguide of the Cambridge IGCSE® Computer Science 0478 pseudocode, however it has the leisure of implementing more operations if required.

You can read the styleguide [here](https://askpakchairul.files.wordpress.com/2015/05/0478_pseudocode_guide.pdf) . You can also find the changes we've made from their styleguide in `docs/changes.md`.

If you're a more advanced user, we recommend reading `docs/grammar.ebnf`, which lists the keywords of `pcse`.

If you prefer learning by example, you can check out `examples`.

## Getting Started

If you haven't already, complete the installation process found in `README.md`. 

Keywords in pcse are in full uppercase. Let's write our very first pcse program using the `OUTPUT` keyword, naming the file `hello.pcse`.

```
// This is a comment. The program will not be able to read this.
// The keyword OUTPUT tells the program to say "Hello, world!"
OUTPUT "Hello, world!"
```

We can run this file by typing in the terminal `./pcse hello.pcse` then hitting enter.

Anything following `//` will be ignored by pcse, so we can write whatever we want after those. The `OUTPUT` keyword will write to the terminal whatever follows it, in this case, "Hello, world!"

We can also tell pcse to output numbers:

```
OUTPUT 1
OUTPUT 1.0
```

This will print `1` and `1.0`. As you can see, `1` and `1.0` are different from each other. The first will be printed without the decimal, but the second one will have the decimal. That is because they are of different types.

### Data Types

Data can be represented through the following ways in pcse:

- An `INTEGER` stores whole numbers, like `1`, `2`, and `-3`.

- A `REAL` can store whole numbers and numbers with a fractional part, like `0.5`, `-0.5`, and `0.0`.

- A `CHAR` stores a single character (both uppercase and lowercase), like `'A'`, `'b'`, and `'@'`. These are identified within single quotes.

- A `STRING` stores a sequence of `CHAR`s, like `"Hello"`. The string stored is identified within double quotes.

- A `BOOLEAN` can be represented by either of two values: `TRUE` or `FALSE`.

### Variables

We can create variables to store our data through the following:

```
DECLARE <identifier>: <type>
```

`DECLARE` is the keyword used to create variables. The `identifier` is the name of the variable that we'll use to store data, and `type` is a data type from the list above. For example:

```
DECLARE name: STRING
```

This creates a variable called `name` which stores a `STRING`. We can assign it a value through the following syntax:

```
<identifier> <- <value>
```

The `identifier` is the name of an existing variable, and `value` is the data we want to store in it. Following our previous example, here's how we'd assign `name` to store `"John Smith"`.

```
name <- "John Smith"
```

We can then use `OUTPUT` to print the value stored in the variable.

```
OUTPUT name
```

Check `examples/03-variables.pcse` for more examples on different variable types.

### Documentation is still in progress...
