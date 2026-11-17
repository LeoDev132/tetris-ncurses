#include <ncurses.h>

enum PIECE_ID
{
	T,
	L,
	J,
	O,
	Z,
	S
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
	wmove(window, y, x*2);
	waddch(window, 'o' | COLOR_PAIR(colorIndex));
	waddch(window, 'o' | COLOR_PAIR(colorIndex));
}

void drawPiece(WINDOW* window, int x, int y, int rot, struct Piece piece, int clear) // if clear is 1, black is used to clear the piece
{
	int ox = piece.origin%piece.size;
	int oy = piece.origin/piece.size;
	x -= ox;
	y -= oy;
	for(int i = 0; i < piece.size * piece.size; i++)
	{
		if(1<<i & piece.shape)
			drawSquare(window, x+i%piece.size, y+i/piece.size, clear ? 7 : piece.colorPairIndex);
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
			if(grid[x+y*10+squareX + squareY*piece.size] != 0)
				return 1;
			if(x+squareX < 1 || x+squareX > 9) return 1;
			if(y+squareY < 1 || y+squareY > 20) return 1;
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
	int data[3][5][2] = 
	{
		{ {}, {}, {}, {}, {} },
	};
	oldRot += 1; // offseting because L is technically -1
	rot += 1;
	for(int i = 0; i < 5; i++)
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

int main()
{	
	int rot = 0;
	struct Piece T;
	T.shape = 58; // nombre binaire pour représenter la piece
	T.size = 3;
	T.colorPairIndex = 4; 
	T.origin = 4;
	struct Piece currPiece = T;
	int ch;
	initscr();
	noecho();
	WINDOW* win = newwin(0, 0, 0, 0);
	WINDOW* subWin = subwin(win, 22, 22, 1, 1);
	start_color();
	init_pair(1, COLOR_RED, COLOR_RED);
	init_pair(2, COLOR_BLUE, COLOR_BLUE);
	init_pair(3, COLOR_GREEN, COLOR_GREEN);
	init_pair(4, COLOR_MAGENTA, COLOR_MAGENTA);
	init_pair(5, COLOR_YELLOW, COLOR_YELLOW);
	init_pair(6, COLOR_CYAN, COLOR_CYAN);
	init_pair(7, COLOR_BLACK, COLOR_BLACK);
	refresh();
	int i = 0;
	int xPos = 5;
	int yPos = 2;
	int grid[200] = {0};
	while(1)
	{
		
		drawPiece(subWin, xPos, yPos, 0, currPiece, 1);
		box(subWin, 0, 0);
		timeout(1);
		ch = getch();
		if(ch == 100 && !checkCollision(currPiece, grid, xPos + 1, yPos))
			xPos++;
		else if(ch == 113 && !checkCollision(currPiece, grid, xPos -1, yPos))
			xPos--;
		else if(ch == 115 && !checkCollision(currPiece, grid, xPos, yPos+1))
			yPos++;
		else if(ch == 108)
		{
			int shapeBuffer = currPiece.shape;
			rotateShapeccw(&currPiece.shape, currPiece.size);
			applyWallkick(currPiece, grid, &xPos, &yPos, rot, rot-1);
			if(checkCollision(currPiece, grid, xPos, yPos))
				currPiece.shape = shapeBuffer;
			else
				rot--;
		}
		else if(ch == 109)
		{
			int shapeBuffer = currPiece.shape;
			rotateShapecw(&currPiece.shape, currPiece.size);
			applyWallkick(currPiece, grid, &xPos, &yPos, rot, rot+1);
			if(checkCollision(currPiece, grid, xPos, yPos))
				currPiece.shape = shapeBuffer;
			else
				rot++;
		}
		if(rot == 3) rot = -1;
		if(rot == -2) rot = 2;

		drawPiece(subWin, xPos, yPos, 0, currPiece, 0);
		wmove(subWin, 1, 1);
		wrefresh(subWin);
	}
	endwin();
}
