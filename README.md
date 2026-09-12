# 🟩 Terminal Wordle

A beautiful Wordle clone written in pure C for the terminal.

It features the real Wordle color palette, a fancy gradient title, animated letter reveals, an on-screen keyboard, and a clear explanation of the tile colors.

No external libraries. No GUI. Just C, ANSI escape codes, and terminal magic.

---

## ✨ Features

- 🎯 Classic Wordle gameplay
- 🟩 Green / 🟨 Yellow / ⬛ Gray tile feedback
- 🎬 Animated letter-by-letter reveal with flash effect
- ⌨️ On-screen keyboard that updates with your guesses
- 🌈 Fancy gradient WORDLE title
- 📖 How To Play screen with color explanation
- 🧠 Compact in-game legend
- 🏆 Wordle-style win messages:
  - Genius
  - Magnificent
  - Impressive
  - Splendid
  - Great
  - Phew
- 🔁 Play again loop
- 🚫 No external dependencies

---

## 📸 Gameplay

You have 6 attempts to guess a hidden 5-letter word.

After each guess, the tiles show your progress:

| Color  | Meaning                                               |
| ------ | ----------------------------------------------------- |
| Green  | The letter is in the word and in the correct position |
| Yellow | The letter is in the word but in the wrong position   |
| Gray   | The letter is not in the word                         |

---

## 🧱 Requirements

- Linux or macOS terminal
- GCC
- Truecolor-capable terminal

Most modern terminals support truecolor, including GNOME Terminal, Kitty, Alacritty, WezTerm, tmux with truecolor support, and many others.

---

## 🚀 Build

Clone the repository and enter the folder:

```bash
git clone https://github.com/Everything-Terminal/wordle_game.git
cd wordle_game
```

Compile:

```bash
gcc -std=c11 -Wall -Wextra wordle.c -o wordle
```

Or with GNU extensions:

```bash
gcc -std=gnu11 -Wall -Wextra wordle.c -o wordle
```

---

## 🎮 Run

```bash
./wordle
```

---

## 🕹 Controls

| Input     | Action             |
| --------- | ------------------ |
| 5 letters | Type your guess    |
| Enter     | Submit the guess   |
| q         | Quit               |
| y / n     | Play again or exit |

---

## 🧠 How It Works

The game uses:

- C arrays for the board and keyboard state
- Enums for tile states
- ANSI truecolor escape codes for colors
- Cursor movement and line clearing for flicker-free rendering
- `nanosleep` for the reveal animation
- A two-pass algorithm for correct duplicate-letter handling

The duplicate-letter logic works like real Wordle:

1. First pass: mark exact green matches.
2. Second pass: mark remaining letters as yellow if they still exist elsewhere in the target word.

---

## 🎨 Colors

The tiles use truecolor ANSI backgrounds inspired by the original Wordle palette:

- Green: `rgb(83, 141, 78)`
- Yellow: `rgb(181, 159, 59)`
- Gray: `rgb(58, 58, 60)`
- Empty tile: `rgb(22, 22, 24)`
- Flash effect: `rgb(255, 255, 255)`

The title uses a custom green-to-rose gradient.

---

## 📁 Project Structure

```text
terminal-wordle/
├── wordle.c
├── README.md
└── .gitignore
```

---

## 🛠 Possible Future Improvements

- Larger word list
- Dictionary validation for guesses
- Hard mode
- Letter-by-letter typing instead of full-word input
- Shake animation for invalid words
- Saved statistics
- Daily word mode
- Sound effects

---

## 🎥 Watch the Full Build on YouTube

Watch the full ASMR coding session here:

👉 [Watch on YouTube](https://www.youtube.com/watch?v=KKE_xsJOVI4)
