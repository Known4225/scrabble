#include "include/turtle.h"
#include "include/ribbon.h"
#include "include/win32Tools.h"
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h" // THANK YOU https://github.com/nothings/stb
/* scrabble dictionary from: https://github.com/zeisler/scrabble/blob/master/db/dictionary.csv */

extern inline int randomInt(int lowerBound, int upperBound) { // random integer between lower and upper bound (inclusive)
    return (rand() % (upperBound - lowerBound + 1) + lowerBound);
}

extern inline double randomDouble(double lowerBound, double upperBound) { // random double between lower and upper bound
    return (rand() * (upperBound - lowerBound) / RAND_MAX + lowerBound); // probably works idk
}

typedef struct { // scrabble
    double c[24]; // color theme
    int cpt; // color select
    double mx; // mouse x
    double my; // mouse y
    double mw; // mouse wheel
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
    signed char holding; // what tile you're holding in your mouse

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
    self.boardX = -150;
    self.boardY = 150;
    self.barX = -70;
    self.barY = -160;

    /* scrabble */
    self.turn = 0;
    self.holding = -1;

    *selfp = self;
}

void scrabbleInit(scrabble *selfp) {
    scrabble self = *selfp;
    double initColors[24] = {
        125, 101, 81, // background color (brown)
        255, 216, 156, // tile color (lighter brown)
        198, 141, 99, // alt tile color (darker)
        226, 223, 218, // board square color (off white)
        80, 133, 198, // double letter (light blue)
        1, 55, 162, // triple letter (dark blue)
        174, 38, 48, // double word (red)
        217, 68, 46 // triple word (orange)
    };
    memcpy(self.c, initColors, sizeof(double) * 24);
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
    *selfp = self;
}

/* lookup a value */
int scrabbleLookup(scrabble *selfp, char letter) {
    if (letter == ' ') {
        return 0;
    }
    return selfp -> table[letter - 65];
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
        itoa(value, toWrite, 10);
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

/* mouse motion */
void mouseTick(scrabble *selfp) {
    scrabble self = *selfp;
    if (turtleMouseDown()) {
        if (self.keys[0] == 0) {
            self.keys[0] = 1;
            /* check if mouse in bar */
            if (self.turn == 0 &&
            self.mx > (self.barX - 10 + self.sx) * self.ss && self.mx < (self.barX + 7 * 20 - 10 + self.sx) * self.ss &&
            self.my > (self.barY - 10 + self.sy) * self.ss && self.my < (self.barY + 10 + self.sy) * self.ss) {
                int index = round((self.mx - (self.barX + self.sx) * self.ss) / (20 * self.ss));
                // printf("index: %d\n", index);
                if (self.yourTiles[index] != 0) {
                    self.holding = index;
                } else {
                    self.focalX = self.mx;
                    self.focalY = self.my;
                    self.focalCSX = self.sx;
                    self.focalCSY = self.sy;
                }
            } else {
                if (self.holding != -1) {
                    if (self.mx > (self.boardX - 10 + self.sx) * self.ss && self.my < (self.boardY + 10 + self.sy) * self.ss &&
                    self.mx < (self.boardX - 10 + 15 * 20 + self.sx) * self.ss && self.my > (self.boardY + 10 - 15 * 20 + self.sy) * self.ss) {
                        int xpos = round((self.mx - (self.boardX + self.sx) * self.ss) / (20 * self.ss));
                        int ypos = round(((self.boardY + self.sy) * self.ss - self.my) / (20 * self.ss));
                        // printf("%d %d\n", xpos, ypos);
                        // self.board[ypos * 15 + xpos] = self.yourTiles[self.holding];
                        list_append(self.pendingTiles, (unitype) self.yourTiles[self.holding], 'c');
                        list_append(self.pendingTiles, (unitype) xpos, 'i');
                        list_append(self.pendingTiles, (unitype) ypos, 'i');
                        self.yourTiles[self.holding] = 0;
                    }
                    self.holding = -2;
                } else {
                    self.focalX = self.mx;
                    self.focalY = self.my;
                    self.focalCSX = self.sx;
                    self.focalCSY = self.sy;
                }
            }
        } else {
            if (self.holding == -1) {
                self.sx = (self.mx - self.focalX) / self.ss + self.focalCSX;
                self.sy = (self.my - self.focalY) / self.ss + self.focalCSY;
            }
        }
    } else {
        if (self.keys[0] != 0) {
            if (self.turn != 0 ||
            self.my > (self.boardY + 10 - 15 * 20 + self.sy) * self.ss) {
                self.holding = -1;
            }
            self.keys[0] = 0;
        }
    }
    if (self.holding > -1) {
        if (self.mx > (self.boardX - 10 + self.sx) * self.ss && self.my < (self.boardY + 10 + self.sy) * self.ss &&
        self.mx < (self.boardX - 10 + 15 * 20 + self.sx) * self.ss && self.my > (self.boardY + 10 - 15 * 20 + self.sy) * self.ss) {
            renderTile(&self, round((self.mx / self.ss - self.sx - 10) / 20) * 20 + 10, round((self.my / self.ss - self.sy - 10) / 20) * 20 + 10, self.yourTiles[self.holding], 0);
        } else {
            renderTile(&self, self.mx / self.ss - self.sx, self.my / self.ss - self.sy, self.yourTiles[self.holding], 0);
        }
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
    window = glfwCreateWindow(1280, 720, "Scrabble", NULL, NULL);
    if (!window) {
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, GLFW_DONT_CARE, GLFW_DONT_CARE, 1280, 720);
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