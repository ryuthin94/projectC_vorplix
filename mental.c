/*
projectC_vorplix
codename usage : Ayrton
*/

/*
  Mining Game by Ayrton
  License: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
  You are free to share and adapt this code under the following terms:
    - Attribution: Give appropriate credit to the author.
    - NonCommercial: Do not use this code for commercial purposes.
    - ShareAlike: Distribute any adaptations under the same license.
  Full license text: https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
*/

/*
  Full Text Mining Game with Save/Load (human-readable)
  - WASD movement (type "end" to go back to menu and save)
  - Persistent save file "save.txt" (human-readable)
  - Multiple ores with depth-based rarity
  - Inventory, Sell, Shop, Help
  - Proper full redraw (no ghost rows)
  - New Game / Continue options

  Author: Ayrton + guidance (ChatGPT Geekforce Reddit etc.)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define CLEAR_CMD "cls"
#else
#define CLEAR_CMD "clear"
#endif

#define SAVE_FILE "save.txt"
#define MAX_ROWS 200
#define MAX_COLS 20

// Game defaults
int cols = 5;
int rows = 5;
int playerX = 0, playerY = 0;
char mine[MAX_ROWS][MAX_COLS];

int coal = 0, iron = 0, gold = 0, diamond = 0, crystal = 0;
int coins = 0;
int pickaxeLevel = 1;
int maxEnergy = 10;
int energy = 10;

// Utility: small delay in seconds (fractional not needed)
void delay_seconds(double sec) {
    clock_t start = clock();
    double ticks = sec * (double)CLOCKS_PER_SEC;
    while ((double)(clock() - start) < ticks);
}

// Animated print (small flavor; pass 0.0 for no delay)
void animated_print(const char *s, double delay_per_char) {
    if (delay_per_char <= 0.0) {
        printf("%s", s);
        return;
    }
    for (size_t i = 0; i < strlen(s); ++i) {
        putchar(s[i]);
        fflush(stdout);
        delay_seconds(delay_per_char);
    }
}

// Clear whole screen and pad with a few newlines to avoid leftovers
void clearScreen() {
    int ret = system(CLEAR_CMD);
    (void)ret;
    // extra padding so leftover lines cannot remain
    for (int i = 0; i < 2; ++i) putchar('\n');
}

// Random ore generation with depth scaling:
// '#' = stone (common, not collectible)
// 'C' = coal
// 'I' = iron
// 'G' = gold
// 'D' = diamond
// 'X' = rare crystal (very rare)
char randomOreForDepth(int depthRowIndex) {
    // depthRowIndex: 0 is top, grows with rows
    int depthFactor = depthRowIndex / 5; // increases slowly with depth
    int r = rand() % 1000; // 0..999 for more granularity

    // Base thresholds at shallow depth
    int stone_th = 600;  // 60.0%
    int coal_th  = 850;  // next 25.0% -> 85.0%
    int iron_th  = 940;  // next 9.0%  -> 94.0%
    int gold_th  = 985;  // next 4.5%  -> 98.5%
    int diamond_th = 998; // next 1.3% -> 99.8%
    // rest -> crystal ~0.2%

    // adjust thresholds to make rarer ores slightly more common with depth
    // deeper -> decrease stone_th and coal_th a bit, increase rare chances
    stone_th  = stone_th  - depthFactor * 10;  // up to -something
    coal_th   = coal_th   - depthFactor * 6;
    iron_th   = iron_th   - depthFactor * 4;
    gold_th   = gold_th   - depthFactor * 2;
    diamond_th= diamond_th- depthFactor * 1;

    // clamp
    if (stone_th < 350) stone_th = 350;
    if (coal_th < stone_th+50) coal_th = stone_th+50;
    if (iron_th < coal_th+20) iron_th = coal_th+20;
    if (gold_th < iron_th+10) gold_th = iron_th+10;
    if (diamond_th < gold_th+2) diamond_th = gold_th+2;

    if (r < stone_th) return '#';
    if (r < coal_th)  return 'C';
    if (r < iron_th)  return 'I';
    if (r < gold_th)  return 'G';
    if (r < diamond_th) return 'D';
    return 'X';
}

// Ensure we initialize the mine from start..end (end exclusive)
void generateMineRows(int start, int end) {
    if (start < 0) start = 0;
    if (end > MAX_ROWS) end = MAX_ROWS;
    for (int i = start; i < end; ++i) {
        for (int j = 0; j < cols; ++j) {
            mine[i][j] = randomOreForDepth(i);
        }
    }
}

// Save game in human-readable file
int saveGame() {
    FILE *f = fopen(SAVE_FILE, "w");
    if (!f) return 0;

    // Header: rows cols playerX playerY pickaxeLevel maxEnergy energy coins
    fprintf(f, "%d %d %d %d %d %d %d %d\n",
            rows, cols, playerX, playerY, pickaxeLevel, maxEnergy, energy, coins);

    // Inventory: coal iron gold diamond crystal
    fprintf(f, "%d %d %d %d %d\n", coal, iron, gold, diamond, crystal);

    // Mine layout: rows lines each with cols char
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            char c = mine[i][j];
            // ensure player not saved as '*' in grid; player is stored separately
            if (i == playerY && j == playerX) c = ' '; // save underlying tile as space
            fputc(c, f);
        }
        fputc('\n', f);
    }

    fclose(f);
    return 1;
}

// Load game; returns 1 if loaded successfully, 0 otherwise
int loadGame() {
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return 0;

    if (fscanf(f, "%d %d %d %d %d %d %d %d\n",
               &rows, &cols, &playerX, &playerY, &pickaxeLevel, &maxEnergy, &energy, &coins) != 8) {
        fclose(f);
        return 0;
    }
    if (fscanf(f, "%d %d %d %d %d\n", &coal, &iron, &gold, &diamond, &crystal) != 5) {
        fclose(f);
        return 0;
    }
    // bounds check
    if (rows < 1 || rows > MAX_ROWS) rows = 5;
    if (cols < 1 || cols > MAX_COLS) cols = 5;
    if (playerX < 0 || playerX >= cols) playerX = 0;
    if (playerY < 0 || playerY >= rows) playerY = 0;

    // Read rows lines
    char lineBuf[MAX_COLS+10];
    for (int i = 0; i < rows; ++i) {
        if (!fgets(lineBuf, sizeof(lineBuf), f)) {
            // missing lines -> fill remaining
            for (int j = 0; j < cols; ++j) mine[i][j] = '#';
            continue;
        }
        // If the line shorter than cols, pad
        int len = (int)strlen(lineBuf);
        int colIdx = 0;
        for (int j = 0; j < cols && j < len; ++j) {
            char ch = lineBuf[j];
            if (ch == '\n' || ch == '\r') break;
            mine[i][colIdx++] = ch;
        }
        while (colIdx < cols) mine[i][colIdx++] = '#';
    }

    fclose(f);
    return 1;
}

// Delete save (used for New Game)
void deleteSaveFile() {
    remove(SAVE_FILE);
}

// Draw mine to screen with full redraw
void drawMine() {
    // Top border
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (i == playerY && j == playerX) {
                putchar('*'); putchar(' ');
            } else {
                putchar(mine[i][j]); putchar(' ');
            }
        }
        putchar('\n');
    }
    putchar('\n');
}

// Collect ore into inventory when stepping on it
void collectOreAt(int r, int c) {
    char t = mine[r][c];
    switch (t) {
        case 'C': coal++; break;
        case 'I': iron++; break;
        case 'G': gold++; break;
        case 'D': diamond++; break;
        case 'X': crystal++; break;
        default: break; // stone '#' or space ' '
    }
    // After collecting, clear tile to space
    mine[r][c] = ' ';
}

// Sell inventory: convert ores to coins
void sellAllOres() {
    int value = coal * 2 + iron * 5 + gold * 12 + diamond * 40 + crystal * 80;
    coins += value;
    coal = iron = gold = diamond = crystal = 0;
    animated_print("Sold all ores.\n", 0.0);
    char msg[80];
    sprintf(msg, "You obtained %d coins.\n", value);
    animated_print(msg, 0.0);
    delay_seconds(0.8);
}

// Shop menu
void shopMenu() {
    while (1) {
        clearScreen();
        printf("===== SHOP =====\n");
        printf("Coins: %d\n", coins);
        printf("1) Upgrade pickaxe (level %d -> %d) : cost %d coins\n", pickaxeLevel, pickaxeLevel+1, 50 + 30 * (pickaxeLevel-1));
        printf("   (Higher pickaxe level reduces energy cost per rare ore - thematic)\n");
        printf("2) Increase max energy (+5) : cost 60 coins\n");
        printf("3) Sell ores here (same as menu Sell)\n");
        printf("4) Back\n");
        printf("=================\n");
        printf("Choice: ");
        int ch = 0;
        if (scanf("%d", &ch) != 1) { while (getchar() != '\n'); ch = 0; }
        if (ch == 1) {
            int cost = 50 + 30 * (pickaxeLevel-1);
            if (coins >= cost) {
                coins -= cost;
                pickaxeLevel++;
                animated_print("Pickaxe upgraded!\n", 0.0);
                delay_seconds(0.6);
            } else {
                animated_print("Not enough coins.\n", 0.0);
                delay_seconds(0.6);
            }
        } else if (ch == 2) {
            if (coins >= 60) {
                coins -= 60;
                maxEnergy += 5;
                animated_print("Max energy increased!\n", 0.0);
                delay_seconds(0.6);
            } else {
                animated_print("Not enough coins.\n", 0.0);
                delay_seconds(0.6);
            }
        } else if (ch == 3) {
            sellAllOres();
            delay_seconds(0.8);
        } else if (ch == 4) {
            return;
        } else {
            animated_print("Invalid choice.\n", 0.0);
            delay_seconds(0.4);
        }
    }
}

// Inventory display
void showInventory() {
    clearScreen();
    printf("===== INVENTORY =====\n");
    printf("Coal: %d\nIron: %d\nGold: %d\nDiamond: %d\nCrystal: %d\n", coal, iron, gold, diamond, crystal);
    printf("Coins: %d\nPickaxe level: %d\nEnergy: %d/%d\n", coins, pickaxeLevel, energy, maxEnergy);
    printf("=====================\n");
    printf("Press Enter to return...");
    while (getchar() != '\n'); // flush leftover
    getchar();
}

// Help screen
void helpMenu() {
    clearScreen();
    printf("===== HELP =====\n");
    printf("W/A/S/D : Move (type lower-case w/a/s/d). Moving consumes 1 energy.\n");
    printf("If you step on an ore (C/I/G/D/X), you pick it up automatically.\n");
    printf("Type 'end' (without quotes) to return to the menu (game will auto-save).\n");
    printf("Shop lets you upgrade pickaxe and energy. Sell ores to get coins.\n");
    printf("Deeper layers have rarer ores. Pickaxe and energy let you go further.\n");
    printf("=================\n");
    printf("Press Enter to return...");
    while (getchar() != '\n'); // flush leftover
    getchar();
}

// Expand mine deeper by adding N extra rows at bottom
void expandMineBy(int extraRows) {
    if (rows + extraRows > MAX_ROWS) extraRows = MAX_ROWS - rows;
    int oldRows = rows;
    rows += extraRows;
    generateMineRows(oldRows, rows);
}

// Game loop
void gameLoop() {
    char input[32];
    srand((unsigned int)time(NULL));

    // Setup: if loaded state may already have mine; otherwise ensure initial mine generated
    if (rows <= 0 || rows > MAX_ROWS) rows = 5;
    if (cols <= 0 || cols > MAX_COLS) cols = 5;
    if (playerX < 0 || playerX >= cols) playerX = 0;
    if (playerY < 0 || playerY >= rows) playerY = 0;
    // if top rows are empty (like '#' not set), ensure generation
    generateMineRows(0, rows);
    energy = (energy > 0 && energy <= maxEnergy) ? energy : maxEnergy;

    while (1) {
        clearScreen();
        printf("===== MINING GAME =====\n\n");
        drawMine();
        printf("Energy: %d/%d   Coins: %d   Pickaxe Lv: %d\n", energy, maxEnergy, coins, pickaxeLevel);
        printf("Inventory: C:%d I:%d G:%d D:%d X:%d\n\n", coal, iron, gold, diamond, crystal);
        printf("Command (w/a/s/d or 'end'): ");
        // read a line safely
        if (scanf("%31s", input) != 1) { while (getchar() != '\n'); continue; }

        if (strcmp(input, "end") == 0) {
            // save and return to menu
            saveGame();
            animated_print("Saving and returning to menu...\n", 0.0);
            delay_seconds(0.6);
            return;
        }

        // movement
        int newX = playerX, newY = playerY;
        if (input[0] == 'w' && playerY > 0) newY--;
        else if (input[0] == 's' && playerY < rows - 1) newY++;
        else if (input[0] == 'a' && playerX > 0) newX--;
        else if (input[0] == 'd' && playerX < cols - 1) newX++;
        else {
            // invalid move or edge
            continue;
        }

        if (energy <= 0) {
            animated_print("You have no energy left. Type 'end' to return to menu and recover.\n", 0.0);
            delay_seconds(0.8);
            continue;
        }

        // step onto tile
        if (mine[newY][newX] != ' ' && mine[newY][newX] != '#') {
            // collectible ore
            collectOreAt(newY, newX);
            // bonus: small coin reward on collection (optional); keeping to collect for sell
        } else if (mine[newY][newX] == '#') {
            // stone - stepping on stone does nothing collectible, but we 'clear' it (simulate digging)
            mine[newY][newX] = ' ';
        }

        // update player pos
        playerX = newX; playerY = newY;

        // consume energy; pickaxeLevel could reduce consumption for large ores - keep simple
        energy--;

        // If near bottom of generated rows, expand
        if (playerY >= rows - 3 && rows < MAX_ROWS) {
            expandMineBy(4); // add a few rows
        }
    }
}

// New game (reset everything)
void newGame() {
    // reset variables
    rows = 5; cols = 5;
    playerX = 0; playerY = 0;
    coal = iron = gold = diamond = crystal = 0;
    coins = 0;
    pickaxeLevel = 1;
    maxEnergy = 10;
    energy = maxEnergy;
    // init mine
    generateMineRows(0, rows);
    // save initial state
    saveGame();
}

// Menu and main
void mainMenu() {
    while (1) {
        clearScreen();
        printf("======= MINING GAME - MENU =======\n");
        printf("1) New Game (resets save)\n");
        printf("2) Continue (load save if exists)\n");
        printf("3) Play (current in-memory game / quick start)\n");
        printf("4) Inventory\n");
        printf("5) Sell Ores\n");
        printf("6) Shop\n");
        printf("7) Help\n");
        printf("8) Exit (saves)\n");
        printf("==================================\n");
        printf("Choice: ");
        int ch = 0;
        if (scanf("%d", &ch) != 1) { while (getchar() != '\n'); ch = 0; }

        if (ch == 1) {
            deleteSaveFile();
            newGame();
            animated_print("New game created. Starting...\n", 0.0);
            delay_seconds(0.6);
            gameLoop();
        } else if (ch == 2) {
            if (loadGame()) {
                animated_print("Save loaded. Resuming...\n", 0.0);
                delay_seconds(0.6);
                gameLoop();
            } else {
                animated_print("No valid save found. Start a new game instead.\n", 0.0);
                delay_seconds(0.8);
            }
        } else if (ch == 3) {
            // Play with current in-memory state; if empty, start new
            if (rows <= 0) newGame();
            gameLoop();
        } else if (ch == 4) {
            showInventory();
        } else if (ch == 5) {
            clearScreen();
            printf("Sell all ores? (y/n): ");
            char ans = 'n';
            while (getchar() != '\n'); // flush
            ans = getchar();
            if (ans == 'y' || ans == 'Y') {
                sellAllOres();
                delay_seconds(0.6);
            }
        } else if (ch == 6) {
            shopMenu();
        } else if (ch == 7) {
            helpMenu();
        } else if (ch == 8) {
            saveGame();
            animated_print("Saved. Goodbye!\n", 0.0);
            delay_seconds(0.5);
            exit(0);
        } else {
            animated_print("Invalid choice.\n", 0.0);
            delay_seconds(0.5);
        }
    }
}

int main() {
    srand((unsigned int)time(NULL));
    // Attempt to load automatically if a save exists (optional)
    if (loadGame()) {
        // keep loaded state but show menu so user can choose
    } else {
        // No save found: initialize a default new game in memory (not saved yet)
        rows = 5; cols = 5; playerX = 0; playerY = 0;
        coal = iron = gold = diamond = crystal = 0;
        coins = 0; pickaxeLevel = 1; maxEnergy = 10; energy = maxEnergy;
        generateMineRows(0, rows);
    }
    mainMenu();
    return 0;
}
