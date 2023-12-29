# OpenPageGetter

`OpenPageGetter` is a C language program that retrieves HTML content from a specified webpage and displays it on the standard output. This program does not have any external dependencies and should work in environments other than Windows, such as Linux and macOS, without requiring the installation of libraries like libcurl.

## Features

- Retrieves HTML content from a specified URL.
- Displays HTML content on the standard output.
- When using the `-d` option, saves the content to `index.html`.

## How to Compile

To compile the program, execute the following commands:

```bash
mkdir build && cd build
cmake .. 
make
```

## How to Run 
To run the program, use the following command:

```bash
./pg example.com
```

## To Save HTML to a File
To save HTML to a file using the -d option, execute as follows:

```bash
./pg -d example.com
```

## License

OpenPageGetter is released under the BSD 3-Clause License. For details, refer to the license text included in the project's source files.