/*
18.12.23:
Pseudo code for mouseTick routine in scrabble engine
*/

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


typedef struct {
    char hover; // mouseHoverEnum
    int hoverPosition[2]; // hover position, (-1, int) on bar, (x, y) on board
    char mouseMode; // mouseModeEnum
    char mousePiece; // character of piece holding
    double mousePlace[2]; // raw coordinates

    char shuffled;
    char keys[20];
} scrabble;

void shuffleBar(int index) {
    if (index == -1) {
        if (shuffled == 0) {
            shuffled = 1;
            // left align the bar
        }
    } else {
        // left align the bar but leave a 0 at index
    }
}

void handleDrop() {
    if (hover == H_BAR) {
        yourTiles[hoverPosition[1]] = mousePiece;
    }
    if (hover == H_BOARD) {
        if (tileExists(hoverPosition) || pendingExists(hoverPosition)) {
            yourTiles[findFirstIndex()] = mousePiece;
        } else {
            addToPending(mousePiece, hoverPosition);
        }
    }
    if (hover == H_NONE) {
        yourTiles[findFirstIndex()] = mousePiece;
    }
    mouseMode = M_NONE;
    mousePiece = M_NONE;
    mousePlace = NONE;
}

void mouseTick(scrabble *selfp) {
    scrabble self = *selfp;
    hover = H_NONE;
    if (mouseOverBar) {
        hover = H_BAR;
        hoverPosition = (-1, integerPosition); // 0 - 6
    }
    if (mouseOverBoard) {
        hoverPosition = (x, y); // 0, 0 is top left
        hover = H_BOARD;
    }
    if (turtleMouseDown()) {
        if (self.keys[0] == 0) { // firstFrame
            self.keys[0] = 1;
            if (mouseMode == M_PIECE) {
                handleDrop();
            } else {
                if (hover == H_BAR) {
                    mouseMode = M_PIECE;
                    mousePiece = clickedOn;
                    mousePlace = clickedOnCoords;
                }
                if (hover == H_BOARD) {
                    if (clickedOnPending) {
                        mouseMode = PIECE;
                        mousePiece = clickedOn;
                        mousePlace = clickedOnCoords;
                        removeFromPending(clickedOn)
                    } else {
                        setDragConstants();
                    }
                }
                if (hover == H_NONE) {
                    setDragConstants();
                }
            }
        } else {
            if (mouseMode == M_PIECE) {
                if (hover == H_BAR) {
                    shuffleBar(hoverPosition[1]);
                } else {
                    shuffleBar(-1);
                }
                if (hover == H_BOARD) {
                    
                }
            }
            if (mouseMode == M_DRAG) {
                dragScreen();
            }
        }
    } else {
        if (self.keys[0] == 1) { // firstFrame
            self.keys[0] = 0;
            if (mouseMode == M_PIECE) {
                if (mouseInPlace(mousePlace)) {
                    // keep mouse == piece
                } else {
                    handleDrop();
                }
            }
        }
    }
    if (mouseMode == M_PIECE) {
        renderCoords = mouseCoordinates;
        if (hover == H_BAR) {
            renderCoords = lockPieceBar(mouseCoordinates)
        }
        if (hover == H_BOARD) {
            renderCoords = lockPieceBoard(mouseCoordinates)
        }
        renderPiece(mousePiece, renderCoords);
    }
}




/* saved */
void mouseTick(scrabble *selfp) {
    scrabble self = *selfp;
    if (turtleMouseDown()) {
        if (self.keys[0] == 0) {
            /* first frame of click */
            self.keys[0] = 1;
            if (self.turn == 0 &&
            self.mx > (self.barX - 10 + self.sx) * self.ss && self.mx < (self.barX + 7 * 20 - 10 + self.sx) * self.ss &&
            self.my > (self.barY - 10 + self.sy) * self.ss && self.my < (self.barY + 10 + self.sy) * self.ss) {
                /* clicked on bar */
                int ind = round((self.mx - (self.barX + self.sx) * self.ss) / (20 * self.ss));
                // printf("index: %d\n", index);
                if (self.yourTiles[ind] != 0) {
                    /* clicked on piece */
                    if (self.index > -1) {
                        /* already holding piece, swap with other piece */
                        char temp = self.yourTiles[ind];
                        self.yourTiles[ind] = self.holding;
                        self.yourTiles[self.index] = temp;
                        self.index = -2;
                        self.holding = 0;
                        self.toggle = 0;
                    } else {
                        /* pick up piece */
                        self.index = ind;
                        self.holding = self.yourTiles[ind];
                        self.yourTiles[ind] = 0;
                    }
                } else {
                    if (self.index > -1) {
                        /* already holding piece, place in empty slot */
                        self.yourTiles[ind] = self.holding;
                        self.index = -2;
                        self.holding = 0;
                        self.toggle = 0;
                    } else {
                        /* did not click on bar, drag screen */
                        self.focalX = self.mx;
                        self.focalY = self.my;
                        self.focalCSX = self.sx;
                        self.focalCSY = self.sy;
                    }
                }
            } else {
                if (self.index > -1) {
                    /* clicked while holding piece */
                    if (self.mx > (self.boardX - 10 + self.sx) * self.ss && self.my < (self.boardY + 10 + self.sy) * self.ss &&
                    self.mx < (self.boardX - 10 + 15 * 20 + self.sx) * self.ss && self.my > (self.boardY + 10 - 15 * 20 + self.sy) * self.ss) {
                        /* clicked on board */
                        int xpos = round((self.mx - (self.boardX + self.sx) * self.ss) / (20 * self.ss));
                        int ypos = round(((self.boardY + self.sy) * self.ss - self.my) / (20 * self.ss));
                        // printf("%d %d\n", xpos, ypos);
                        // self.board[ypos * 15 + xpos] = self.holding;
                        list_append(self.pendingTiles, (unitype) self.holding, 'c');
                        list_append(self.pendingTiles, (unitype) xpos, 'i');
                        list_append(self.pendingTiles, (unitype) ypos, 'i');
                        self.index = -1;
                        self.holding = 0;
                        self.toggle = 0;
                    } else {
                        /* clicked off of board */
                        self.yourTiles[self.index] = self.holding;
                    }
                    self.index = -2;
                    self.holding = 0;
                    self.toggle = 0;
                } else {
                    int clickedPending = -1;
                    for (int i = 0; i < self.pendingTiles -> length; i += 3) {
                        /* check if you clicked on a pending piece */
                        printf("%0.1lf %0.1lf %0.1lf %0.1lf %0.1lf %0.1lf\n", (self.boardX - 10 + self.pendingTiles -> data[i + 1].i * 20 + self.sx) * self.ss, self.mx, (self.boardX + 10 + self.pendingTiles -> data[i + 2].i * 20 + self.sx) * self.ss,
                        (self.boardY - 10 - self.pendingTiles -> data[i + 1].i * 20 + self.sy) * self.ss, self.my, (self.boardY + 10 - self.pendingTiles -> data[i + 2].i * 20  + self.sy) * self.ss);
                        if (self.mx > (self.boardX - 10 + self.pendingTiles -> data[i + 1].i * 20 + self.sx) * self.ss && self.my > (self.boardY - 10 - self.pendingTiles -> data[i + 2].i * 20 + self.sy) * self.ss &&
                        self.mx < (self.boardX + 10 + self.pendingTiles -> data[i + 1].i * 20 + self.sx) * self.ss && self.my < (self.boardY + 10 - self.pendingTiles -> data[i + 2].i * 20  + self.sy) * self.ss) {
                            clickedPending = i;
                            break;
                        }
                    }
                    if (clickedPending > -1) {
                        /* grab pending pieces from board */
                        printf("clicked: %d\n", clickedPending);
                        self.holding = self.pendingTiles -> data[clickedPending].c;
                        self.toggle = 1;
                        for (int i = 0; i < 7; i++) {
                            self.index = 0;
                            if (self.yourTiles[i] == 0) {
                                self.index = i;
                                break;
                            }
                        }
                        list_delete(self.pendingTiles, clickedPending);
                        list_delete(self.pendingTiles, clickedPending);
                        list_delete(self.pendingTiles, clickedPending);
                    } else {
                        /* drag screen */
                        self.focalX = self.mx;
                        self.focalY = self.my;
                        self.focalCSX = self.sx;
                        self.focalCSY = self.sy;
                    }
                }
            }
        } else {
            if (self.index == -1) {
                self.sx = (self.mx - self.focalX) / self.ss + self.focalCSX;
                self.sy = (self.my - self.focalY) / self.ss + self.focalCSY;
            }
        }
    } else {
        if (self.keys[0] != 0) {
            if (self.index > -1 && (self.turn != 0 ||
            self.my > (self.boardY + 10 - 15 * 20 + self.sy) * self.ss)) {
                if (self.mx > (self.boardX - 10 + self.sx) * self.ss && self.my < (self.boardY + 10 + self.sy) * self.ss &&
                self.mx < (self.boardX - 10 + 15 * 20 + self.sx) * self.ss && self.my > (self.boardY + 10 - 15 * 20 + self.sy) * self.ss) {
                    int xpos = round((self.mx - (self.boardX + self.sx) * self.ss) / (20 * self.ss));
                    int ypos = round(((self.boardY + self.sy) * self.ss - self.my) / (20 * self.ss));
                    list_append(self.pendingTiles, (unitype) self.holding, 'c');
                    list_append(self.pendingTiles, (unitype) xpos, 'i');
                    list_append(self.pendingTiles, (unitype) ypos, 'i');
                    self.index = -1;
                    self.holding = 0;
                    self.toggle = 0;
                } else {
                    self.yourTiles[self.index] = self.holding;
                }
                self.holding = 0;
                self.index = -1;
            }
            self.keys[0] = 0;
        }
    }
    if (self.index > -1) {
        if (self.mx > (self.boardX - 10 + self.sx) * self.ss && self.my < (self.boardY + 10 + self.sy) * self.ss &&
        self.mx < (self.boardX - 10 + 15 * 20 + self.sx) * self.ss && self.my > (self.boardY + 10 - 15 * 20 + self.sy) * self.ss) {
            renderTile(&self, round((self.mx / self.ss - self.sx) / 20) * 20, round((self.my / self.ss - self.sy - 10) / 20) * 20 + 10, self.holding, 0);
        } else {
            renderTile(&self, self.mx / self.ss - self.sx, self.my / self.ss - self.sy, self.holding, 0);
        }
    }
    *selfp = self;
}