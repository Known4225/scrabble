/*
19.12.23:
Ryan Srichai

ToDo:
Play button
Words
 - Newly placed tiles must be along one row or column
 - Newly placed tiles must all be connected... kind of (difficult)
 - List of all new words generated upon hitting submit button
 - each word in the list is checked for validity
 - if all words are valid, each word is scored
Rendering
 - Function to draw red boxes around squares
 - Parameters: x, y, length, direction
 - Words should be checked for validity in real time, and red boxes appear when a word is not valid
Score render

Enemy player
 - generate all legal moves (how?)
 - rank each move by score
 
Additional enemy heuristics
 - rank each move by number of tiles used

Bug:
Sometimes tiles disappear if you drag them too fast in the hotbar
Also I want to fix it dragging when I am trying to click it and move it. Maybe add an exception for the hotbar that allows it to go upward slightly
*/
#include "include/turtle.h"
#include "include/ribbon.h"
#include "include/win32Tools.h"
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h" // THANK YOU https://github.com/nothings/stb (this is for glfw logo on window and in taskbar)
/* scrabble dictionary from: https://github.com/zeisler/scrabble/blob/master/db/dictionary.csv */

extern inline int randomInt(int lowerBound, int upperBound) { // random integer between lower and upper bound (inclusive)
    return (rand() % (upperBound - lowerBound + 1) + lowerBound);
}

extern inline double randomDouble(double lowerBound, double upperBound) { // random double between lower and upper bound
    return (rand() * (upperBound - lowerBound) / RAND_MAX + lowerBound); // probably works idk
}

enum hoverModeEnum {
    H_NONE = 0,
    H_BAR = 1,
    H_BOARD = 2,
};

enum mouseModeEnum {
    M_NONE = 0,
    M_PIECE = 1,
    M_DRAG = 2
};

typedef struct { // scrabble
    double c[27]; // color theme
    int cpt; // color select
    
    double scrollSpeed;
    list_t *allTiles; // all the tiles and point values
    list_t *bag; // the bag of tiles
    list_t *pendingTiles; // ghost tiles
    char yourTiles[7]; // your tiles
    char oppTiles[7]; // your opponents tiles
    char table[26]; // table of values
    char boardSetup[225]; // board special values
    char board[225]; // board tiles
    char turn; // 0 for your turn, 1 for opponent's turn

    /* mouse functionality */
    double mx; // mouse x
    double my; // mouse y
    double mw; // mouse wheel
    char hover; // mouseHoverEnum
    int hoverPosition[2]; // hover position, (-1, int) on bar, (x, y) on board
    char mouseMode; // mouseModeEnum
    char mousePiece; // character of piece holding
    double mousePlace[2]; // raw coordinates
    char shuffled; // prevent shuffling bar when it is not needed
    int prevMouse[4]; // for swapping pending tiles

    /* screen coordinates */
    double sx; // screenX
    double sy; // screenY
    double ss; // screenSize

    double focalX; // for dragging the screen
    double focalY;
    double focalCSX;
    double focalCSY;

    /* keybinds */
    char keys[20];
    /* keybinds:
    0 - mouseDown
    1 - space
    2 - up arrow
    3 - down arrow
    */

   /* positional elements */
   double boardX;
   double boardY;
   double barX;
   double barY;
} scrabble;

void importTiles(scrabble *selfp, char *filename) {
    printf("Attempting to load %s\n", filename);
    list_clear(selfp -> allTiles);
    FILE *fp = fopen(filename, "r");
    char chPar = '\0';
    int intPar1;
    int intPar2;
    int end = 0;
    while (end != EOF) {
        end = fscanf(fp, "%d,%c,%d,\n", &intPar1, &chPar, &intPar2);
        // printf("%d %d %c %d\n", end, intPar1, chPar, intPar2);
        if (end != EOF) {
            for (int i = 0; i < intPar1; i++) {
                list_append(selfp -> allTiles, (unitype) chPar, 'c');
                list_append(selfp -> allTiles, (unitype) intPar2, 'i');
            }
            if (chPar != ' ') {
                selfp -> table[chPar - 65] = intPar2;
            }
        }
    }
    // list_print(selfp -> allTiles);
    fclose(fp);
    printf("loaded %d tiles from %s\n", selfp -> allTiles -> length / 2, filename);
}

void scrabbleExport(scrabble *selfp, char *filename) {

}

void resetGame(scrabble *selfp) {
    scrabble self = *selfp;
    /* refill bag */
    list_copy(self.allTiles, self.bag);
    list_clear(self.pendingTiles);
    /* generate random tiles */
    for (int i = 0; i < 7; i++) {
        int picked = randomInt(0, self.bag -> length / 2 - 1) * 2;
        self.yourTiles[i] = self.bag -> data[picked].c;
        list_delete(self.bag, picked);
        list_delete(self.bag, picked);
    }
    for (int i = 0; i < 7; i++) {
        int picked = randomInt(0, self.bag -> length / 2 - 1) * 2;
        self.oppTiles[i] = self.bag -> data[picked].c;
        list_delete(self.bag, picked);
        list_delete(self.bag, picked);
    }
    for (int i = 0; i < 225; i++) {
        self.board[i] = 0;
    }

    /* screen */
    self.ss = 1;
    self.sx = 0;
    self.sy = 0;
    self.boardX = -140;
    self.boardY = 150;
    self.barX = -60;
    self.barY = -160;

    /* scrabble */
    self.turn = 0;
    

    /* mouse */
    self.hover = H_NONE;
    self.hoverPosition[0] = H_NONE;
    self.hoverPosition[1] = H_NONE;
    self.mouseMode = M_NONE;
    self.mousePiece = M_NONE;
    self.mousePlace[0] = M_NONE;
    self.mousePlace[1] = M_NONE;
    self.shuffled = 0;
    self.prevMouse[0] = 0;

    *selfp = self;
}

void scrabbleInit(scrabble *selfp) {
    scrabble self = *selfp;
    double initColors[27] = {
        125, 101, 81, // background color (brown)
        255, 216, 156, // tile color (lighter brown)
        198, 141, 99, // alt tile color (darker)
        226, 223, 218, // board square color (off white)
        80, 133, 198, // double letter (light blue)
        1, 55, 162, // triple letter (dark blue)
        174, 38, 48, // double word (red)
        217, 68, 46, // triple word (orange)
        255, 0, 0 // outline color (red)
    };
    memcpy(self.c, initColors, sizeof(double) * 27);
    self.cpt = 0;
    turtleBgColor(self.c[self.cpt], self.c[self.cpt + 1], self.c[self.cpt + 2]);
    self.allTiles = list_init(); // initialise list
    self.bag = list_init();
    self.pendingTiles = list_init();
    importTiles(&self, "include/scrabbleTiles.csv"); // import tiles from file "scrabbleTiles.csv"
    srand(time(NULL)); // initialise random seed
    resetGame(&self);

    char boardPlace[225] = {
        4, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 4,
        0, 3, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 3, 0,
        0, 0, 3, 0, 0, 0, 1, 0, 1, 0, 0, 0, 3, 0, 0,
        1, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 1,
        0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0,
        0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0,
        0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0,
        4, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 1, 0, 0, 4,
        0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0,
        0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0,
        0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0,
        1, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 1,
        0, 0, 3, 0, 0, 0, 1, 0, 1, 0, 0, 0, 3, 0, 0,
        0, 3, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 3, 0,
        4, 0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 1, 0, 0, 4
    }; // the board
    memcpy(self.boardSetup, boardPlace, 225);
    self.scrollSpeed = 1.08;

    /* keyboard */
    for (int i = 0; i < 20; i++) {
        self.keys[i] = 0;
    }
    *selfp = self;
}

/* lookup a value */
int scrabbleLookup(scrabble *selfp, char letter) {
    if (letter == ' ') {
        return 0;
    }
    return selfp -> table[letter - 65];
}

/* retrieve the tile at the board position, returns 0 if not found */
char retrieveTile(scrabble *selfp, int x, int y) {
    if (selfp -> board[y * 15 + x] == 0) {
        return -1;
    }
    return selfp -> board[y * 15 + x];
}

/* retrieve the pending tile at the board position, returns -1 if not found */
char retrievePendingTile(scrabble *selfp, int x, int y) {
    for (int i = 0; i < selfp -> pendingTiles -> length; i += 3) {
        if (selfp -> pendingTiles -> data[i + 1].i == x && selfp -> pendingTiles -> data[i + 2].i == y) {
            return i;
        }
    }
    return -1;
}

/* render a single tile */
void renderTile(scrabble *selfp, double x, double y, char letter, int col) {
    int value = scrabbleLookup(selfp, letter);
    double red;
    double green;
    double blue;
    if (col) {
        red = selfp -> c[selfp -> cpt + 6];
        green = selfp -> c[selfp -> cpt + 7];
        blue = selfp -> c[selfp -> cpt + 8];
    } else {
        red = selfp -> c[selfp -> cpt + 3];
        green = selfp -> c[selfp -> cpt + 4];
        blue = selfp -> c[selfp -> cpt + 5];
    }
    turtleQuad((x - 9 + selfp -> sx) * selfp -> ss, (y - 9 + selfp -> sy) * selfp -> ss, 
    (x + 9 + selfp -> sx) * selfp -> ss, (y - 9 + selfp -> sy) * selfp -> ss, 
    (x + 9 + selfp -> sx) * selfp -> ss, (y + 9 + selfp -> sy) * selfp -> ss, 
    (x - 9 + selfp -> sx) * selfp -> ss, (y + 9 + selfp -> sy) * selfp -> ss, 
    red, green, blue);
    char toWrite[12];
    toWrite[0] = letter;
    toWrite[1] = '\0';
    turtlePenColor(0, 0, 0);
    textGLSetWeight(30);
    textGLWriteString(toWrite, (x + selfp -> sx) * selfp -> ss, (y + selfp -> sy) * selfp -> ss, 10 * selfp -> ss, 50);
    if (value != 0) {
        sprintf(toWrite, "%d", value);
        textGLWriteString(toWrite, (x + 7 + selfp -> sx) * selfp -> ss, (y - 6 + selfp -> sy) * selfp -> ss, 4 * selfp -> ss, 100);
    }
    textGLSetWeight(20);
}

/* render your tiles */
void renderBar(scrabble *selfp) {
    for (int i = 0; i < 7; i++) {
        if (selfp -> yourTiles[i] != 0) {
            renderTile(selfp, selfp -> barX + 20 * i, selfp -> barY, selfp -> yourTiles[i], 0);
        }
    }
}

/* render the board */
void renderBoard(scrabble *selfp) {
    scrabble self = *selfp;
    int x = self.boardX;
    int y = self.boardY;
    char toWrite[3];
    toWrite[2] = '\0';
    for (int i = 0; i < 15; i++) {
        x = self.boardX;
        for (int j = 0; j < 15; j++) {
            if (self.board[i * 15 + j] == 0) {
                int colMod = 3 * self.boardSetup[i * 15 + j];
                turtleQuad((x - 9 + self.sx) * self.ss, (y - 9 + self.sy) * self.ss, 
                (x + 9 + self.sx) * self.ss, (y - 9 + self.sy) * self.ss, 
                (x + 9 + self.sx) * self.ss, (y + 9 + self.sy) * self.ss, 
                (x - 9 + self.sx) * self.ss, (y + 9 + self.sy) * self.ss, 
                self.c[self.cpt + 9 + colMod], self.c[self.cpt + 10 + colMod], self.c[self.cpt + 11 + colMod]);
                turtlePenColor(self.c[self.cpt + 9], self.c[self.cpt + 10], self.c[self.cpt + 11]);
                textGLSetWeight(30);
                switch (colMod) {
                    case 0:
                    break;
                    case 3:
                    toWrite[0] = 'D';
                    toWrite[1] = 'L';
                    textGLWriteString(toWrite, (x + self.sx) * self.ss, (y + self.sy) * self.ss, 6 * self.ss, 50);
                    break;
                    case 6:
                    toWrite[0] = 'T';
                    toWrite[1] = 'L';
                    textGLWriteString(toWrite, (x + self.sx) * self.ss, (y + self.sy) * self.ss, 6 * self.ss, 50);
                    break;
                    case 9:
                    toWrite[0] = 'D';
                    toWrite[1] = 'W';
                    textGLWriteString(toWrite, (x + self.sx) * self.ss, (y + self.sy) * self.ss, 6 * self.ss, 50);
                    break;
                    case 12:
                    toWrite[0] = 'T';
                    toWrite[1] = 'W';
                    textGLWriteString(toWrite, (x + self.sx) * self.ss, (y + self.sy) * self.ss, 6 * self.ss, 50);
                    break;
                    default:
                    break;
                }
                textGLSetWeight(20);
            } else {
                renderTile(&self, x, y, self.board[i * 15 + j], 0);
            }
            x += 20;
        }
        y -= 20;
    }
    for (int i = 0; i < self.pendingTiles -> length; i += 3) {
        renderTile(&self, (self.boardX + 20 * self.pendingTiles -> data[i + 1].i), (self.boardY - 20 * self.pendingTiles -> data[i + 2].i), self.pendingTiles -> data[i].c, 1);
    }

    *selfp = self;
}

/* renders a red outline around tiles */
void renderOutline(scrabble *selfp, int x, int y, int dir, int len) {
    /* direction:
    0 - up
    1 - right
    2 - down
    3 - left */
    scrabble self = *selfp;
    int deltaX;
    int deltaY;
    switch (dir) {
        case 0:
        deltaX = 0;
        deltaY = 1;
        break;
        case 1:
        deltaX = 1;
        deltaY = 0;
        break;
        case 2:
        deltaX = 0;
        deltaY = -1;
        break;
        case 3:
        deltaX = -1;
        deltaY = 0;
        break;
        default:
        break;
    }
    double gotoX = (self.boardX + 20 * (x - 0.5 * deltaX - 0.5 * deltaY) + self.sx) * self.ss;
    double gotoY = (self.boardY - 20 * (y + 0.5 * deltaY - 0.5 * deltaX) + self.sy) * self.ss;
    turtlePenColor(self.c[self.cpt + 24], self.c[self.cpt + 25], self.c[self.cpt + 26]);
    turtlePenSize(self.ss * 5);
    turtleGoto(gotoX, gotoY);
    turtlePenDown();
    gotoX += (20 * (len + 1) * deltaX) * self.ss;
    gotoY += (20 * (len + 1) * deltaY) * self.ss;
    turtleGoto(gotoX, gotoY);
    turtlePenUp();
}

/* mouse functions */

/* returns the integer aligned with the bar at the raw coordinates, -1 if not on bar */
int onBar(scrabble *selfp, double x, double y) {
    if (x > (selfp -> barX - 10 + selfp -> sx) * selfp -> ss && x < (selfp -> barX + 7 * 20 - 10 + selfp -> sx) * selfp -> ss &&
        y > (selfp -> barY - 10 + selfp -> sy) * selfp -> ss && y < (selfp -> barY + 10 + selfp -> sy) * selfp -> ss) {
            return round((x - (selfp -> barX + selfp -> sx) * selfp -> ss) / (20 * selfp -> ss));
        }
    return -1;
}

/* returns the integer aligned with the board at the raw coordinates, -1 if not on board */
int onBoard(scrabble *selfp, double x, double y) {
    if (x > (selfp -> boardX - 10 + selfp -> sx) * selfp -> ss && y < (selfp -> boardY + 10 + selfp -> sy) * selfp -> ss &&
        x < (selfp -> boardX - 10 + 15 * 20 + selfp -> sx) * selfp -> ss && y > (selfp -> boardY + 10 - 15 * 20 + selfp -> sy) * selfp -> ss) {
            int xpos = round((x - (selfp -> boardX + selfp -> sx) * selfp -> ss) / (20 * selfp -> ss));
            int ypos = round(((selfp -> boardY + selfp -> sy) * selfp -> ss - y) / (20 * selfp -> ss));
            return ypos * 15 + xpos;
        }
    return -1;
}

/* convert a (0 - 6) to a physical coordinate */
double convertBarX(scrabble *selfp, int x) {
    return (selfp -> barX + 20 * x + selfp -> sx) * selfp -> ss;
}

/* y position of bar */
double convertBarY(scrabble *selfp, int y) {
    return (selfp -> barY + selfp -> sy) * selfp -> ss; // it's always the same
}

/* convert a boardX (0 - 14) to a physical coordinate */
double convertBoardX(scrabble *selfp, int x) {
    return (selfp -> boardX + 20 * x + selfp -> sx) * selfp -> ss;
}

/* convert a boardY (0 - 14) to a physical coordinate */
double convertBoardY(scrabble *selfp, int y) {
    return (selfp -> boardY - 20 * y + selfp -> sx) * selfp -> ss;
}

/* return 1 if the mouse is within tile_size coordinates of the location (check if mouse in tile) */
char mouseInPlace(scrabble *selfp, double x, double y) {
    // printf("%lf %lf %lf %lf\n", fabs(x - selfp -> mx), fabs(y - selfp -> my));
    if (fabs(x - selfp -> mx) > 10 * selfp -> ss || fabs(y - selfp -> my) > 10 * selfp -> ss) {
        return 0;
    }
    return 1;
}

/* reorganise bar (left pad with a gap) */
void shuffleBar(scrabble *selfp, int index) {
    if (index == -1) {
        if (selfp -> shuffled == 0) {
            selfp -> shuffled = 1;
            // left align the bar
            char shuffle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            int next = 0;
            for (int i = 0; i < 7; i++) {
                if (selfp -> yourTiles[i] != 0) {
                    shuffle[next] = selfp -> yourTiles[i];
                    next++;
                }
            }
            memcpy(selfp -> yourTiles, shuffle, 7);
        }
    } else {
        // left align the bar but leave a 0 at index
        selfp -> shuffled = 0;
        char shuffle[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        int next = 0;
        for (int i = 0; i < 7; i++) {
            if (selfp -> yourTiles[i] != 0) {
                shuffle[next] = selfp -> yourTiles[i];
                next++;
            }
        }
        for (int i = next; i > index; i--) {
            shuffle[i] = shuffle[i - 1];
        }
        shuffle[index] = 0;
        memcpy(selfp -> yourTiles, shuffle, 7);
    }
}

/* find first 0 in yourTiles */
int findFirstIndex(scrabble *selfp) {
    int first = 0;
    for (int i = 0; i < 7; i++) {
        if (selfp -> yourTiles[i] == 0) {
            first = i;
            break;
        }
    }
    return first;
}

/* code when a piece is dropped or placed */
void handleDrop(scrabble *selfp) {
    if (selfp -> hover == H_BAR) {
        // printf("barDrop\n");
        selfp -> yourTiles[selfp -> hoverPosition[0]] = selfp -> mousePiece;
    }
    if (selfp -> hover == H_BOARD) {
        // printf("boardDrop %d %d\n", retrieveTile(selfp, selfp -> hoverPosition[0], selfp -> hoverPosition[1]), retrievePendingTile(selfp, selfp -> hoverPosition[0], selfp -> hoverPosition[1]));
        int pender = retrievePendingTile(selfp, selfp -> hoverPosition[0], selfp -> hoverPosition[1]);
        if (retrieveTile(selfp, selfp -> hoverPosition[0], selfp -> hoverPosition[1]) != -1) {
            selfp -> yourTiles[findFirstIndex(selfp)] = selfp -> mousePiece;
        } else if (pender != -1) {
            // replace pending tile with mousePiece
            char temp = selfp -> pendingTiles -> data[pender].c;
            selfp -> pendingTiles -> data[pender].c = selfp -> mousePiece;
            if (selfp -> prevMouse[0] == 1) {
                // swap with prevMouse
                list_append(selfp -> pendingTiles, (unitype) temp, 'c');
                list_append(selfp -> pendingTiles, (unitype) selfp -> prevMouse[2], 'i');
                list_append(selfp -> pendingTiles, (unitype) selfp -> prevMouse[3], 'i');
            } else {
                // swap with bar piece
                selfp -> yourTiles[findFirstIndex(selfp)] = temp;
            }
        } else {
            list_append(selfp -> pendingTiles, (unitype) selfp -> mousePiece, 'c');
            list_append(selfp -> pendingTiles, (unitype) selfp -> hoverPosition[0], 'i');
            list_append(selfp -> pendingTiles, (unitype) selfp -> hoverPosition[1], 'i');
        }
    }
    if (selfp -> hover == H_NONE) {
        selfp -> yourTiles[findFirstIndex(selfp)] = selfp -> mousePiece;
    }
    selfp -> mouseMode = M_NONE;
    selfp -> mousePiece = M_NONE;
    selfp -> mousePlace[0] = M_NONE;
    selfp -> mousePlace[1] = M_NONE;
}

/* driver mouseTick */
void mouseTick(scrabble *selfp) {
    scrabble self = *selfp;
    self.hover = H_NONE;
    int barCoord = onBar(&self, self.mx, self.my);
    if (barCoord != -1) {
        self.hover = H_BAR;
        self.hoverPosition[0] = barCoord; // 0 - 6
        self.hoverPosition[1] = -1; // there is no y position since bar is 1 dimensional
    }
    int boardCoord = onBoard(&self, self.mx, self.my);
    if (boardCoord != -1) {
        self.hoverPosition[0] = boardCoord % 15;
        self.hoverPosition[1] = boardCoord / 15; // 0, 0 is top left
        self.hover = H_BOARD;
    }
    // printf("hover: %d mouseMode: %d\n", self.hover, self.mouseMode);
    if (turtleMouseDown()) {
        if (self.keys[0] == 0) {
            /* firstFrame */
            // printf("firstFrame\n");
            self.keys[0] = 1;
            if (self.mouseMode == M_PIECE) {
                handleDrop(&self);
            } else {
                if (self.hover == H_BAR) {
                    self.shuffled = 0;
                    self.prevMouse[0] = 0;
                    self.mouseMode = M_PIECE;
                    self.mousePiece = self.yourTiles[self.hoverPosition[0]];
                    self.mousePlace[0] = convertBarX(&self, self.hoverPosition[0]);
                    self.mousePlace[1] = convertBarY(&self, self.hoverPosition[1]);
                    self.yourTiles[self.hoverPosition[0]] = 0;
                }
                if (self.hover == H_BOARD) {
                    int pender = retrievePendingTile(&self, self.hoverPosition[0], self.hoverPosition[1]);
                    // printf("bX: %d bY: %d pend: %d\n", self.hoverPosition[0], self.hoverPosition[1], pender);
                    if (pender != -1) {
                        self.mouseMode = M_PIECE;
                        self.mousePiece = self.pendingTiles -> data[pender].c;
                        self.mousePlace[0] = convertBoardX(&self, self.hoverPosition[0]);
                        self.mousePlace[1] = convertBoardY(&self, self.hoverPosition[1]);
                        /* set prevMouse */
                        self.prevMouse[0] = 1;
                        self.prevMouse[1] = self.mousePiece;
                        self.prevMouse[2] = self.pendingTiles -> data[pender + 1].i; 
                        self.prevMouse[3] = self.pendingTiles -> data[pender + 2].i;
                        /* delete from pending tiles */
                        list_delete(self.pendingTiles, pender);
                        list_delete(self.pendingTiles, pender);
                        list_delete(self.pendingTiles, pender);
                    } else {
                        /* set drag constants */
                        self.prevMouse[0] = 0;
                        self.mouseMode = M_DRAG;
                        self.focalX = self.mx;
                        self.focalY = self.my;
                        self.focalCSX = self.sx;
                        self.focalCSY = self.sy;
                    }
                }
                if (self.hover == H_NONE) {
                    /* set drag constants */
                    self.mouseMode = M_DRAG;
                    self.focalX = self.mx;
                    self.focalY = self.my;
                    self.focalCSX = self.sx;
                    self.focalCSY = self.sy;
                }
            }
        } else {
            if (self.mouseMode == M_PIECE) {
                if (self.hover == H_BAR) {
                    // printf("shuffle bar %d\n", self.hoverPosition[0]);
                    // for (int i = 0; i < 7; i++) {
                    //     printf("%d, ", self.yourTiles[i]);
                    // }
                    // printf("\n");
                    shuffleBar(&self, self.hoverPosition[0]);
                } else {
                    shuffleBar(&self, -1);
                }
                if (self.hover == H_BOARD) {
                    
                }
            }
            if (self.mouseMode == M_DRAG) {
                /* drag screen */
                self.sx = (self.mx - self.focalX) / self.ss + self.focalCSX;
                self.sy = (self.my - self.focalY) / self.ss + self.focalCSY;
            }
        }
    } else {
        if (self.keys[0] == 1) {
            /* firstFrame */
            self.keys[0] = 0;
            if (self.mouseMode == M_PIECE) {
                if (mouseInPlace(&self, self.mousePlace[0], self.mousePlace[1])) {
                    // keep mouse == piece
                    // printf("in place\n");
                } else {
                    // printf("dropped\n");
                    handleDrop(&self);
                }
            }
            if (self.mouseMode == M_DRAG) {
                self.mouseMode = M_NONE;
            }
        } else {
            if (self.mouseMode == M_PIECE) {
                if (self.hover == H_BAR) {
                    shuffleBar(&self, self.hoverPosition[0]);
                } else {
                    shuffleBar(&self, -1);
                }
            }
        }
    }
    if (self.mouseMode == M_PIECE) {
        int renderCoords[2];
        renderCoords[0] = self.mx / self.ss - self.sx;
        renderCoords[1] = self.my / self.ss - self.sy;
        if (self.hover == H_BAR) {
            // renderCoords = lockPieceBar(mouseCoordinates);

        }
        if (self.hover == H_BOARD) {
            // renderCoords = lockPieceBoard(mouseCoordinates);
            renderCoords[0] = round((self.mx / self.ss - self.sx) / 20) * 20;
            renderCoords[1] = round((self.my / self.ss - self.sy - 10) / 20) * 20 + 10;
        }
        renderTile(&self, renderCoords[0], renderCoords[1], self.mousePiece, 0);
    }
    *selfp = self;
}

void scrollTick(scrabble *selfp) {
    scrabble self = *selfp;
    if (self.mw > 0) {
        self.sx -= (turtle.mouseX * (-1 / self.scrollSpeed + 1)) / self.ss;
        self.sy -= (turtle.mouseY * (-1 / self.scrollSpeed + 1)) / self.ss;
        self.ss *= self.scrollSpeed;
    }
    if (self.mw < 0) {
        self.ss /= self.scrollSpeed;
        self.sx += (turtle.mouseX * (-1 / self.scrollSpeed + 1)) / self.ss;
        self.sy += (turtle.mouseY * (-1 / self.scrollSpeed + 1)) / self.ss;
    }
    *selfp = self;
}

/* key presses */
void hotkeyTick(scrabble *selfp) {
    scrabble self = *selfp;
    if (turtleKeyPressed(GLFW_KEY_SPACE)) {
        if (self.keys[1] == 0) {
            self.keys[1] = 1;
            resetGame(&self);
        }
    } else {
        self.keys[1] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_UP)) {
        if (self.keys[2] == 0) {
            self.keys[2] = 1;
        }
    } else {
        self.keys[2] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_DOWN)) {
        if (self.keys[3] == 0) {
            self.keys[3] = 1;
        }
    } else {
        self.keys[3] = 0;
    }
    *selfp = self;
}

/* ribbon functionality */
void parseRibbonOutput(scrabble *selfp) {
    scrabble self = *selfp;
    if (ribbonRender.output[0] == 1) {
        ribbonRender.output[0] = 0; // untoggle
        if (ribbonRender.output[1] == 0) { // file
            if (ribbonRender.output[2] == 1) { // new
                printf("New Game\n");
                resetGame(&self);
                strcpy(win32FileDialog.filename, "null");
            }
            if (ribbonRender.output[2] == 2) { // save
                if (strcmp(win32FileDialog.filename, "null") == 0) {
                    if (win32FileDialogPrompt(1, "") != -1) {
                        printf("Saved to: %s\n", win32FileDialog.filename);
                        scrabbleExport(&self, win32FileDialog.filename);
                    }
                } else {
                    printf("Saved to: %s\n", win32FileDialog.filename);
                    scrabbleExport(&self, win32FileDialog.filename);
                }
            }
            if (ribbonRender.output[2] == 3) { // save as
                if (win32FileDialogPrompt(1, "") != -1) {
                    printf("Saved to: %s\n", win32FileDialog.filename);
                    scrabbleExport(&self, win32FileDialog.filename);
                }
            }
            if (ribbonRender.output[2] == 4) { // load
                if (win32FileDialogPrompt(0, "") != -1) {
                    // printf("Loaded data from: %s\n", win32FileDialog.filename);
                    resetGame(&self);
                    importTiles(&self, "scrabbleTiles.csv");
                }
            }
        }
        if (ribbonRender.output[1] == 1) { // edit
            if (ribbonRender.output[2] == 1) { // undo
                printf("undo\n");
            }
            if (ribbonRender.output[2] == 2) { // redo
                printf("redo\n");
            }
        }
        if (ribbonRender.output[1] == 2) { // view
            if (ribbonRender.output[2] == 1) { // appearance
                printf("appearance settings\n");
            } 
            if (ribbonRender.output[2] == 2) { // GLFW
                printf("GLFW settings\n");
            } 
        }
    }
    *selfp = self;
}

int main(int argc, char *argv[]) {
    GLFWwindow* window;
    /* Initialize glfw */
    if (!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA (Anti-Aliasing) with 4 samples (must be done before window is created (?))

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1920, 1080, "Scrabble", glfwGetPrimaryMonitor(), NULL);
    if (!window) {
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, GLFW_DONT_CARE, GLFW_DONT_CARE, 1920, 1080);
    gladLoadGL();
    /* load logo */
    GLFWimage icon;
    int iconChannels;
    unsigned char *iconPixels = stbi_load("include/ScrabbleIcon.jpg", &icon.width, &icon.height, &iconChannels, 4); // 4 color channels for RGBA
    icon.pixels = iconPixels;
    glfwSetWindowIcon(window, 1, &icon);

    /* initialize turtle */
    turtleInit(window, -320, -180, 320, 180);
    /* initialise textGL */
    textGLInit(window, "include/fontBez.tgl");
    /* initialise ribbon */
    ribbonInit(window, "include/ribbonConfig.txt");
    ribbonLightTheme();

    /* initialise win32Tools */
    win32ToolsInit();
    win32FileDialogAddExtension("txt"); // add txt to extension restrictions

    int tps = 60; // ticks per second (locked to fps in this case)
    clock_t start, end;
    scrabble self;
    scrabbleInit(&self); // initialise the scrabble

    while (turtle.close == 0) {
        start = clock(); // for frame syncing
        turtleGetMouseCoords(); // get the mouse coordinates
        if (turtle.mouseX > 320) { // bound mouse coordinates to window coordinates
            self.mx = 320;
        } else {
            if (turtle.mouseX < -320) {
                self.mx = -320;
            } else {
                self.mx = turtle.mouseX;
            }
        }
        if (turtle.mouseY > 180) {
            self.my = 180;
        } else {
            if (turtle.mouseY < -180) {
                self.my = -180;
            } else {
                self.my = turtle.mouseY;
            }
        }
        self.mw = turtleMouseWheel();
        if (self.keys[2]) // up arrow
            self.mw += 1;
        if (self.keys[3]) // down arrow
            self.mw -= 1;
        turtleClear();
        renderBar(&self);
        renderBoard(&self);
        renderOutline(&self, 7, 7, 3, 4);
        mouseTick(&self);
        hotkeyTick(&self);
        scrollTick(&self);
        
        
        // ribbonDraw();
        parseRibbonOutput(&self);
        turtleUpdate(); // update the screen
        end = clock();
        while ((double) (end - start) / CLOCKS_PER_SEC < (1 / (double) tps)) {
            end = clock();
        }
    }
}