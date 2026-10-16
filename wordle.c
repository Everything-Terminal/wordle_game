#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define WORD_LEN 5
#define MAX_GUESSES 6

#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"

#define GREEN_BG    "\033[48;2;83;141;78m"
#define YELLOW_BG   "\033[48;2;181;159;59m"
#define GRAY_BG     "\033[48;2;58;58;60m"
#define EMPTY_BG    "\033[48;2;22;22;24m"
#define FLASH_BG    "\033[48;2;255;255;255m"
#define KEY_BG      "\033[48;2;129;131;132m"
#define WHITE_FG    "\033[38;2;255;255;255m"
#define BLACK_FG    "\033[38;2;20;20;20m"

#define TITLE_COLOR "\033[1;38;2;255;255;255m"
#define HUD_COLOR   "\033[38;2;170;172;178m"
#define PROMPT_COLOR  "\033[1;38;2;181;159;59m"
#define ERROR_COLOR   "\033[1;38;2;230;90;90m"
#define SUCCESS_COLOR "\033[1;38;2;120;200;120m"

#define CURSOR_HOME   "\033[H"
#define CLEAR_TO_EOL  "\033[K"
#define CLEAR_TO_END  "\033[J"
#define HIDE_CURSOR   "\033[?25l"
#define SHOW_CURSOR   "\033[?25h"

typedef enum { UNKNOWN, ABSENT, PRESENT, EXACT } LetterState;

const char *words[] = {
    "apple", "brave", "candy", "delta", "eagle", "flame",
    "grape", "heart", "ivory", "joker", "kayak", "lemon",
    "music", "noble", "opera", "piano", "quiet", "river",
    "solar", "tiger", "unity", "vivid", "whale", "youth",
    "zebra", "crash", "dream", "earth", "frost", "globe",
    "hover", "input", "juice", "light", "magic", "ocean",
    "plant", "quest", "robot", "storm", "trust", "world"
};

#define WORD_COUNT (sizeof(words) / sizeof(words[0]))

/* ------ Dictionary (valid guesses) ------ */
static char (*dict)[WORD_LEN + 1] = NULL;
static size_t dictCount = 0;
static size_t dictCap = 0;
static bool haveDictionary = false;

static void dictAdd(const char *w) {
    if (dictCount == dictCap) {
        size_t newCap = dictCap ? dictCap * 2 : 1024;
        void *p = realloc(dict, newCap * sizeof(*dict));
        if (!p) return;
        dict = p;
        dictCap = newCap;
    }
    memcpy(dict[dictCount], w, WORD_LEN + 1);
    dictCount++;
}

static int dictCompare(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

/* Loads lowercase 5-letter words from a file. Returns number added. */
static size_t dictLoadFile(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    size_t before = dictCount;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strcspn(line, "\r\n");
        line[len] = '\0';
        if (len != WORD_LEN) continue;

        bool ok = true;
        for (int i = 0; i < WORD_LEN; i++) {
            if (line[i] < 'a' || line[i] > 'z') { ok = false; break; }
        }
        if (ok) dictAdd(line);
    }
    fclose(f);
    return dictCount - before;
}

static void dictInit(void) {
    /* Your own list wins; otherwise use the system dictionary. */
    size_t n = dictLoadFile("words.txt");
    if (n == 0) n = dictLoadFile("/usr/share/dict/words");
    haveDictionary = (n > 0);

    /* Answers are always valid guesses. */
    for (size_t i = 0; i < WORD_COUNT; i++) dictAdd(words[i]);

    qsort(dict, dictCount, sizeof(*dict), dictCompare);
}

static bool inDictionary(const char *guess) {
    if (!haveDictionary) {
        /* No word list available: fall back to accepting any 5 letters,
         * unless it is one of our answers (always fine). */
        return true;
    }
    return bsearch(guess, dict, dictCount, sizeof(*dict), dictCompare) != NULL;
}

static bool cursorHidden = false;

void showCursor(void) {
    if (cursorHidden) {
        printf(SHOW_CURSOR);
        fflush(stdout);
        cursorHidden = false;
    }
}

void hideCursor(void) {
    if (!cursorHidden) {
        printf(HIDE_CURSOR);
        fflush(stdout);
        cursorHidden = true;
    }
}

void cleanup(void) {
    showCursor();
}

void sleepMs(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void clearScreen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void chooseWord(char *target) {
    int index = rand() % (int)WORD_COUNT;
    strcpy(target, words[index]);
}

void initKeyboard(LetterState keyboard[26]) {
    for (int i = 0; i < 26; i++) {
        keyboard[i] = UNKNOWN;
    }
}

void removeNewline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
    len = strlen(s);
    if (len > 0 && s[len - 1] == '\r') {
        s[len - 1] = '\0';
    }
}

bool validGuess(const char *guess) {
    if (strlen(guess) != WORD_LEN) {
        return false;
    }
    for (int i = 0; i < WORD_LEN; i++) {
        if (!isalpha((unsigned char)guess[i])) {
            return false;
        }
    }
    return true;
}

void evaluateGuess(const char *target, const char *guess, LetterState result[WORD_LEN]) {
    int remaining[26] = {0};

    for (int i = 0; i < WORD_LEN; i++) {
        if (guess[i] == target[i]) {
            result[i] = EXACT;
        } else {
            result[i] = UNKNOWN;
            remaining[target[i] - 'a']++;
        }
    }

    for (int i = 0; i < WORD_LEN; i++) {
        if (result[i] == EXACT) {
            continue;
        }
        int idx = guess[i] - 'a';
        if (remaining[idx] > 0) {
            result[i] = PRESENT;
            remaining[idx]--;
        } else {
            result[i] = ABSENT;
        }
    }
}

void updateKeyboard(const char *guess,
                    const LetterState result[WORD_LEN],
                    LetterState keyboard[26]) {
    for (int i = 0; i < WORD_LEN; i++) {
        int idx = guess[i] - 'a';
        if (result[i] == EXACT) {
            keyboard[idx] = EXACT;
        } else if (result[i] == PRESENT) {
            if (keyboard[idx] != EXACT) {
                keyboard[idx] = PRESENT;
            }
        } else if (result[i] == ABSENT) {
            if (keyboard[idx] == UNKNOWN) {
                keyboard[idx] = ABSENT;
            }
        }
    }
}

void drawTile(const char *bg, const char *fg, char letter, bool filled) {
    if (filled) {
        printf("%s%s%s %c %s ", bg, fg, BOLD,
                (char)toupper((unsigned char)letter), RESET);
    } else {
        printf("%s   %s ", bg, RESET);
    }
}

void drawStateTile(LetterState s, char letter) {
    switch (s) {
        case EXACT:   drawTile(GREEN_BG, WHITE_FG, letter, true);  break;
        case PRESENT: drawTile(YELLOW_BG, WHITE_FG, letter, true); break;
        case ABSENT:  drawTile(GRAY_BG, WHITE_FG, letter, true);   break;
        default:      drawTile(EMPTY_BG, WHITE_FG, letter, true);  break;
    }
}

void drawKey(LetterState s, char letter) {
    switch (s) {
        case EXACT:   drawTile(GREEN_BG, WHITE_FG, letter, true);  break;
        case PRESENT: drawTile(YELLOW_BG, WHITE_FG, letter, true); break;
        case ABSENT:  drawTile(GRAY_BG, WHITE_FG, letter, true);   break;
        default:      drawTile(KEY_BG, BLACK_FG, letter, true);    break;
    }
}

/* Fancy gradient-tile title*/
void printFancyTitle(void) {
    static const int c[6][3] = {
        { 88, 178, 132 },
        { 122, 184, 96 },
        { 176, 188, 84 },
        { 214, 178, 86 },
        { 226, 148, 92 },
{ 214, 116, 122 }
    };
    const char *t = "WORDLE";

    printf("        ");
    for (int i = 0; i < 6; i++) {
        printf("\033[48;2;%d;%d;%dm\033[38;2;255;255;255m\033[1m %c \033[0m ",
               c[i][0], c[i][1], c[i][2], t[i]);
    }
}

void printHeader(void) {
    printf(CLEAR_TO_EOL "\n");
    printFancyTitle();
    printf(RESET CLEAR_TO_EOL "\n");
    printf(CLEAR_TO_EOL "\n");
}

/* Compact in-game legend so the player always knows the colors */
void printLegend(void) {
    printf("        ");
    drawTile(GREEN_BG, WHITE_FG, 'A', true);
    printf(HUD_COLOR " right spot   " RESET);
    drawTile(YELLOW_BG, WHITE_FG, 'B', true);
    printf(HUD_COLOR " wrong spot   " RESET);
    drawTile(GRAY_BG, WHITE_FG, 'C', true);
    printf(HUD_COLOR " absent" RESET);
}

void renderGame(const char guesses[][WORD_LEN + 1],
                const LetterState results[][WORD_LEN],
                int filledRows,
                int animRow,
                int revealCount,
                int flashIndex,
                const LetterState keyboard[26],
                const char *status) {
    printf(CURSOR_HOME);
    printHeader();

    for (int r = 0; r < MAX_GUESSES; r++) {
        printf("          ");
        for (int c = 0; c < WORD_LEN; c++) {
            if (r < filledRows) {
                drawStateTile(results[r][c], guesses[r][c]);
            } else if (r == animRow) {
                if (c < revealCount) {
                    drawStateTile(results[r][c], guesses[r][c]);
                } else if (c == flashIndex) {
                    drawTile(FLASH_BG, BLACK_FG, guesses[r][c], true);
                } else {
                    drawTile(EMPTY_BG, WHITE_FG, guesses[r][c], true);
                }
            } else {
                drawTile(EMPTY_BG, WHITE_FG, ' ', false);
            }
        }
        printf(RESET CLEAR_TO_EOL "\n");
    }

    printf(CLEAR_TO_EOL "\n");

    printLegend();
    printf(RESET CLEAR_TO_EOL "\n");

    printf(CLEAR_TO_EOL "\n");

    const char *rows[3] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
    int indents[3] = { 1, 3, 7 };

    for (int r = 0; r < 3; r++) {
        for (int s = 0; s < indents[r]; s++) {
            printf(" ");
        }

        for (int i = 0; rows[r][i] != '\0'; i++) {
            char ch = rows[r][i];
            drawKey(keyboard[ch - 'A'], ch);
        }
        printf(RESET CLEAR_TO_EOL "\n");
    }

    printf(CLEAR_TO_EOL "\n");

    printf("%s", status);
    printf(CLEAR_TO_EOL CLEAR_TO_END);
    fflush(stdout);
}

void printInstructions(void) {
  clearScreen();

  printf(CLEAR_TO_EOL "\n");
  printFancyTitle();
  printf(RESET CLEAR_TO_EOL "\n");
  printf(CLEAR_TO_EOL "\n");

  printf(TITLE_COLOR "        HOW TO PLAY" RESET CLEAR_TO_EOL "\n");
  printf(CLEAR_TO_EOL "\n");
  printf(HUD_COLOR "        Guess the hidden 5-letter word in 6 tries." RESET CLEAR_TO_EOL "\n");
  printf(HUD_COLOR "        After each guess, the tile colors show how close you are:" RESET CLEAR_TO_EOL "\n");
  printf(CLEAR_TO_EOL "\n");

  printf("          ");
  drawTile(GREEN_BG, WHITE_FG, 'A', true);
  printf(HUD_COLOR " Green  - the letter is in the word and in the right spot" RESET CLEAR_TO_EOL "\n");

  printf("          ");
  drawTile(YELLOW_BG, WHITE_FG, 'B', true);
  printf(HUD_COLOR " Yellow - the letter is in the word but in the wrong spot" RESET CLEAR_TO_EOL "\n");

  printf("          ");
  drawTile(GRAY_BG, WHITE_FG, 'C', true);
  printf(HUD_COLOR " Gray   - the letter is not in the word at all" RESET CLEAR_TO_EOL "\n");

  printf(CLEAR_TO_EOL "\n");
  printf(HUD_COLOR "        Type your guess and press Enter. Type q to quit." RESET CLEAR_TO_EOL "\n");
  printf(CLEAR_TO_EOL "\n");
  printf(PROMPT_COLOR "        Press Enter to start..." RESET);
  fflush(stdout);
}

void animateGuess(char guesses[][WORD_LEN + 1],
                  LetterState results[][WORD_LEN],
                  int row,
                  LetterState keyboard[26]) {
  hideCursor();

  renderGame(guesses, results, row, row, 0, -1, keyboard, "");
  sleepMs(250);

  for (int c = 0; c < WORD_LEN; c++) {
      renderGame(guesses, results, row, row, c, c, keyboard, "");
      sleepMs(120);
      renderGame(guesses, results, row, row, c + 1, -1, keyboard, "");
      sleepMs(200);
  }

  updateKeyboard(guesses[row], results[row], keyboard);
  renderGame(guesses, results, row + 1, -1, 0, -1, keyboard, "");
  sleepMs(100);

  showCursor();
}

int main(void) {
    atexit(cleanup);
    srand((unsigned int)time(NULL));

    char input[64];
    bool play = true;

    dictInit();

    printInstructions();
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }

    while (play) {
        clearScreen();

        char target[WORD_LEN + 1];
        char guesses[MAX_GUESSES][WORD_LEN + 1];
        LetterState results[MAX_GUESSES][WORD_LEN];
        LetterState keyboard[26];

        chooseWord(target);
        initKeyboard(keyboard);

        int guessCount = 0;
        bool won = false;

        while (guessCount < MAX_GUESSES) {
            char prompt[128];
            snprintf(prompt, sizeof(prompt),
                     PROMPT_COLOR "        Guess %d/%d > " RESET,
                     guessCount + 1, MAX_GUESSES);

            renderGame(guesses, results, guessCount, -1, 0, -1, keyboard, prompt);
            showCursor();

            if (fgets(input, sizeof(input), stdin) == NULL) {
                cleanup();
                return 0;
            }
            removeNewline(input);

            if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
                clearScreen();
                showCursor();
                printf(TITLE_COLOR "\n  Thanks for playing!\n\n" RESET);
                return 0;
            }

            if (!validGuess(input)) {
                char err[128];
                snprintf(err, sizeof(err),
                         ERROR_COLOR "        Enter exactly 5 letters (A-Z)." RESET);
                renderGame(guesses, results, guessCount, -1, 0, -1, keyboard, err);
                sleepMs(1300);
                continue;
            }

            char guess[WORD_LEN + 1];
            for (int i = 0; i < WORD_LEN; i++) {
                guess[i] = (char)tolower((unsigned char)input[i]);
            }
            guess[WORD_LEN] = '\0';

            if (!inDictionary(guess)) {
                char err[128];
                snprintf(err, sizeof(err),
                         ERROR_COLOR "        Not in word list." RESET);
                renderGame(guesses, results, guessCount, -1, 0, -1, keyboard, err);
                sleepMs(1300);
                continue;
            }

            strcpy(guesses[guessCount], guess);
            evaluateGuess(target, guess, results[guessCount]);

            animateGuess(guesses, results, guessCount, keyboard);

            bool allExact = true;
            for (int i = 0; i < WORD_LEN; i++) {
                if (results[guessCount][i] != EXACT) {
                    allExact = false;
                    break;
                }
            }

            guessCount++;

            if (allExact) {
                won = true;
                break;
            }
        }
            char endMsg[128];
            if (won) {
                const char *m;
                switch (guessCount) {
                    case 1: m = "Genius!"; break;
                    case 2: m = "Magnificent!"; break;
                    case 3: m = "Impressive!"; break;
                    case 4: m = "Splendid!"; break;
                    case 5: m = "Great!"; break;
                    default: m = "Phew!"; break;
                }
                snprintf(endMsg, sizeof(endMsg),
                         SUCCESS_COLOR "        %s Solved in %d/%d." RESET,
                         m, guessCount, MAX_GUESSES);
            } else {
                char upper[WORD_LEN + 1];
                for (int i = 0; i < WORD_LEN; i++) {
                    upper[i] = (char)toupper((unsigned char)target[i]);
                }
                upper[WORD_LEN] = '\0';
                snprintf(endMsg, sizeof(endMsg),
                         ERROR_COLOR "        The word was: %s" RESET, upper);
            }

            renderGame(guesses, results, guessCount, -1, 0, -1, keyboard, endMsg);
            printf("\n");
            printf(SUCCESS_COLOR "        Play again? (y/n): " RESET);
            fflush(stdout);
            showCursor();

            if (fgets(input, sizeof(input), stdin) == NULL) {
                break;
            }
            if (!(input[0] == 'y' || input[0] == 'Y')) {
                play = false;
            }
        }

        clearScreen();
        showCursor();
        printf(TITLE_COLOR "\n  Thanks for playing!\n\n" RESET);
        return 0;
    
}
