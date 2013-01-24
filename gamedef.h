#pragma once

#include "StdAfx.h"
#include <stdio.h>

static const int NUMROWS = 4;
static const int NUMCOLS = 3;
static const int NUMCONS = 3;

#ifdef _DEBUG
static const int FRAMESPERSEC = 50;
#else
static const int FRAMESPERSEC = 10;
#endif
typedef struct
{
	float x;
	float y;
}CIRCLE;

struct checker {
	int rgb[3];
	int x;
	int y;
};
struct player {
	int _color;
	int color[3];
	bool human;
};
struct Point {
	int x;
	int y;
};
struct Move {
	int row;
	int col;
	int moveNumber;
	int color;
	std::string comment;
};

static const int EMPTY = 2;
static const int RED = 0;
static const int BLACK = 1;

std::string format_arg_list(const char *fmt, va_list args);
std::string format(const char *fmt, ...);

