# WiM

WiM is a small terminal text editor written in C. It is intentionally simple:
the project is a compact place to learn about terminal input, text buffers, and
editor state.

## Build

WiM requires a C11 compiler and a POSIX terminal. Build it with:

```sh
make
```

The build enables common compiler warnings. Run the non-interactive regression
tests with:

```sh
make test
```

Remove generated files with `make clean`.

## Usage

Open an existing file or prepare a new file for editing:

```sh
./wim path/to/file.txt
./wim path/to/new-file.txt
```

Running `./wim` without a filename opens an unnamed buffer. The unnamed buffer
can be edited, but it must be opened with a filename to be saved.

## Controls

### Normal mode

| Key | Action |
| --- | --- |
| `i` | Enter insert mode |
| `h` / `j` / `k` / `l` | Move left / down / up / right |
| Arrow keys | Move the cursor |
| `0` / `$` | Move to the start / end of the line |
| `x` | Delete the character under the cursor |
| `n` or `Ctrl-S` | Save the current file |
| `q` | Quit |

The original `w`, `a`, `s`, and `d` movement keys are also supported.

### Insert mode

Type text normally. `Enter` splits the current line, `Backspace` joins or
deletes text, and `Escape` returns to normal mode.

## Project status

WiM is an intentionally small learning project rather than a replacement for
Vim or Neovim. It currently focuses on reliable line editing, basic terminal
navigation, and safe file loading/saving.
