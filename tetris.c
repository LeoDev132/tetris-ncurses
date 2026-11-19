#include <ncurses.h>
//TODO harddrop + randomize bag + hold + preview next pieces
enum PIECE_ID
{
	T,
	L,
	J,
	O,
	Z,
	S
};

enum COLOR_ID
{
	UNUSED,
	RED,
	BLUE,
	GREEN,
	MAGENTA,
	YELLOW,
	CYAN,
	BLACK,
	WHITE
};

struct Piece
{
	int id;
	int shape;
	int size;
	int colorPairIndex;
	int origin;
};

void drawSquare(WINDOW* window, int x, int y, int colorIndex)
{
	wmove(window, y+1, (x+1)*2);
	waddch(window, 'o' | COLOR_PAIR(colorIndex));
	waddch(window, 'o' | COLOR_PAIR(colorIndex));
}

void drawPiece(WINDOW* window, int x, int y, int rot, struct Piece piece, int clear) // if clear is 1, black is used to clear the piece
{
	int debug = 0;
	int ox = piece.origin%piece.size;
	int oy = piece.origin/piece.size;
	x -= ox;
	y -= oy;
	for(int i = 0; i < piece.size * piece.size; i++)
	{
		if(1<<i & piece.shape)
			drawSquare(window, x+i%piece.size, y+i/piece.size, clear ? 7 : piece.colorPairIndex);
		else if(debug)
			drawSquare(window, x+i%piece.size, y+i/piece.size, clear ? 7 : piece.colorPairIndex-1);
	}
}

void rotateShapeccw(int* shape, int size)
{
	int buffer = *shape;
	*shape = 0;
	int n = size-1;
	for(int i = 0; i < size; i++)
	{
		for(int j = 0; j < size; j++)
		{
			int index = j*size + n-i;
			if(buffer & 1<<index)
				*shape |= (1<<(i*size+j));
		}
	}
}

void rotateShapecw(int* shape, int size)
{
	int buffer = *shape;
	*shape = 0;
	int n = size-1;
	for(int i = 0; i < size; i++)
	{
		for(int j = 0; j < size; j++)
		{
			int index = (n-j)*size + i;
			if(buffer & 1<<index)
				*shape |= (1<<(i*size+j));
		}
	}
}


int checkCollision(struct Piece piece, int* grid, int x, int y)
{
	
	int ox = piece.origin%piece.size;
	int oy = piece.origin/piece.size;
	x -= ox;
	y -= oy;
	for(int i = 0; i < piece.size * piece.size; i++)
	{
		if(1<<i & piece.shape)
		{
			int squareX = i%piece.size;
			int squareY = i/piece.size;
			if(grid[x+y*10+squareX + squareY*10] != 0)
				return 1;
			if(x+squareX < 0 || x+squareX > 10) return 1;
			if(y+squareY < 0 || y+squareY > 20) return 1;
		}
	}
	return 0;
}

struct pair 
{
	int x;
	int y;
};

//https://tetris.wiki/Super_Rotation_System
void applyWallkick(struct Piece piece, int* grid, int* x, int* y, int oldRot, int rot)
{
	int data[4][6][2] = 
	{
		{ {0, 0}, {1, 0}, {-1, 0}, {1, -1}, {0, 2}, {1, 2} }, // L
		{ {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, // 0
		{ {0, 0}, {-1, 0}, {1, 0}, {-1, -1}, {0, 2}, {-1, 2} }, // R
		{ {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, // 2
	};
	oldRot += 1; // offseting because L is technically -1
	rot += 1;
	for(int i = 0; i < 6; i++)
	{
		int oX = data[rot][i][0];
		int oY = data[rot][i][1];
		if(!checkCollision(piece, grid, *x + oX, *y+oY))
		{
			*x += oX;
			*y += oY;
			return;
		}
	}
}

void printToGrid(struct Piece piece, int* grid, int x,  int y)
{

	int ox = piece.origin%piece.size;
	int oy = piece.origin/piece.size;
	x -= ox;
	y -= oy;
	for(int i = 0; i < piece.size * piece.size; i++)
	{
		int index = y * 10 + x + i%piece.size + i/piece.size;
		if(1<<i & piece.shape)
		{
			int squareX = i%piece.size;
			int squareY = i/piece.size;
			grid[x+y*10+squareX + squareY*10] = piece.colorPairIndex;
				
		}
	}
}

void drawGrid(WINDOW* window, int* grid)
{
	for(int i = 0; i < 200; i++)
	{
		if(grid[i] != 0)
		{
			int x = i%10;
			int y = i / 10;
			drawSquare(window, x, y, grid[i]);
		}
	}
}

void setGridSquare(int* grid, int x, int y, int color)
{
	grid[x + y*10] = color;
}

void placePiece(struct Piece* piece, int* grid, int* x, int* y, struct Piece* bag, int* bagIndex)
{

	printToGrid(*piece, grid, *x, *y);
	*x = 5;
	*y = 2;
	*bagIndex = (*bagIndex+1)%7;
	*piece = bag[*bagIndex];
}

void clearLines(WINDOW* window, int* grid)
{
	int linesToClear[4] = {18};
	int clearCount = 0;
	for(int i = 0; i < 20; i++)
	{
		int currLineCount = 0;
		for(int j = 0; j < 10; j++)
		{
			if(grid[i*10+j]!=0)
				currLineCount++;
		}
		if(currLineCount == 10)
		{
			linesToClear[clearCount++] = i;
		}
	}
	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < clearCount; j++)
		{
			grid[linesToClear[j]*10+i] = 0;
			drawSquare(window, i, linesToClear[j], BLACK);
			wrefresh(window);
		}
		napms(40);
	}
}

int main()
{	
	int rot = 0;
	struct Piece T;
	T.shape = 58; // nombre binaire pour représenter la piece
	T.size = 3;
	T.colorPairIndex = MAGENTA; 
	T.origin = 4;
	
	struct Piece S;
	S.shape = 51;
	S.size = 3;
	S.colorPairIndex = RED;
	S.origin = 4;

	struct Piece Z;
	Z.shape = 30;
	Z.size = 3;
	Z.colorPairIndex = GREEN;
	Z.origin = 4;	

	struct Piece J;
	J.shape = 57;
	J.size = 3;
	J.colorPairIndex = WHITE;
	J.origin = 4;

	struct Piece L;
	L.shape = 60;
	L.size = 3;
	L.colorPairIndex = BLUE;
	L.origin = 4;

	struct Piece O;
	O.shape = 15;
	O.size = 2;
	O.colorPairIndex = YELLOW;
	O.origin = 3;
	
	struct Piece I;
	I.shape = 240;
	I.size = 4;
	I.colorPairIndex = CYAN;
	I.origin = 5;

	struct Piece currPiece = S;
	int ch;
	initscr();
	noecho();
	WINDOW* win = newwin(0, 0, 0, 0);
	WINDOW* subWin = subwin(win, 22, 24, 1, 1);
	start_color();
	init_pair(1, COLOR_RED, COLOR_RED);
	init_pair(2, COLOR_BLUE, COLOR_BLUE);
	init_pair(3, COLOR_GREEN, COLOR_GREEN);
	init_pair(4, COLOR_MAGENTA, COLOR_MAGENTA);
	init_pair(5, COLOR_YELLOW, COLOR_YELLOW);
	init_pair(6, COLOR_CYAN, COLOR_CYAN);
	init_pair(7, COLOR_BLACK, COLOR_BLACK);
	init_pair(8, COLOR_WHITE, COLOR_WHITE);
	refresh();
	int i = 0;
	int xPos = 5;
	int yPos = 2;
	int grid[200] = {0};
	struct Piece bag[7] = {T, J, L, S, Z, I, O};
	int bagIndex = 0;
	for(int i = 17; i < 20; i++)
	{
		for(int j = 0; j < 10; j++)
		{
			setGridSquare(grid, j, i, 1);
		}
	}
/*	setGridSquare(grid, 4, 17, 0);
	setGridSquare(grid, 4, 18, 0);
	setGridSquare(grid, 4, 19, 0);
	setGridSquare(grid, 5, 18, 0);
	setGridSquare(grid, 3, 16, 1);
	setGridSquare(grid, 3, 15, 1);
	setGridSquare(grid, 4, 15, 1);*/
	while(1)
	{
		
		drawPiece(subWin, xPos, yPos, 0, currPiece, 1);
		box(subWin, 0, 0);
		timeout(128);
		ch = getch();
		if(ch == 100 && !checkCollision(currPiece, grid, xPos + 1, yPos))
			xPos++;
		else if(ch == 113 && !checkCollision(currPiece, grid, xPos -1, yPos))
			xPos--;
		else if(ch == 115) 
		{
			if(checkCollision(currPiece, grid, xPos, yPos+1))
			{
				placePiece(&currPiece, grid, &xPos, &yPos, bag, &bagIndex);
				clearLines(subWin, grid);
			}
			else yPos++;
		}
		else if(ch == 108)
		{
			int shapeBuffer = currPiece.shape;
			rotateShapeccw(&currPiece.shape, currPiece.size);
			applyWallkick(currPiece, grid, &xPos, &yPos, -1, -1);
			if(checkCollision(currPiece, grid, xPos, yPos))
				currPiece.shape = shapeBuffer;
			else
				rot--;
		}
		else if(ch == 109)
		{
			int shapeBuffer = currPiece.shape;
			rotateShapecw(&currPiece.shape, currPiece.size);
			applyWallkick(currPiece, grid, &xPos, &yPos, 1, 1);
			if(checkCollision(currPiece, grid, xPos, yPos))
				currPiece.shape = shapeBuffer;
			else
				rot++;
		}
		if(rot == 3) rot = -1;
		if(rot == -2) rot = 2;

		/*if(checkCollision(currPiece, grid, xPos, yPos+1))
		{
			placePiece(currPiece, grid, xPos, yPos);
		}*/
		//else yPos++;
		drawGrid(subWin, grid);
		drawPiece(subWin, xPos, yPos, 0, currPiece, 0);
		wmove(subWin, 1, 1);
		wrefresh(subWin);
	}
	endwin();
}
