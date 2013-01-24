#pragma once

#include <fl/gl.h>
#include <fl/glu.h>
#include <fl/glut.h>

class CRender
{
public:
	HDC m_hdc;  // A handle to the device context
	int m_numRows;
	int m_numCols;
	int m_cellSize;
	int m_borderL;
	int m_borderR;
	int m_borderTop;
	int m_borderBot;
	int m_historyW;
	int m_disWidth;
	int m_disHeight;
	int m_boardPosX;
	int m_boardPosY;
	void *m_font;
	float Pi;
	int m_winWidth;
	int m_winHeight;
	CRITICAL_SECTION m_csCheckerList;
	CRITICAL_SECTION m_csMoveList;

	CConnect3AIThread* m_pThreadAI;

	CRender(int numRows, int numCols);
	~CRender(void);

	void CreateRenderContext(HWND hWnd);
	void output(int x, int y, const char *string);
	GLvoid draw_circle(const GLfloat radius,
	                   const GLuint num_vertex,
					   const GLfloat x,
					   const GLfloat y);
	void RenderIt();
	void SetupPixelFormat(HDC hdc);
};
