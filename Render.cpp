// Copyright © 2011 Steve Coward
#include "StdAfx.h"
#include <math.h>
#include <iostream>
#include <list>
#include "Connect3AIThread.h"
#include "gamedef.h"
#include "Render.h"

extern CRITICAL_SECTION g_csCheckerList;
extern CRITICAL_SECTION g_csMoveList;

CRender::CRender(int numRows, int numCols)
{
	Pi=4*atan(1.0f);

	m_font = GLUT_BITMAP_HELVETICA_12;

	m_numRows = numRows;
	m_numCols = numCols;
	m_cellSize = 80;
	m_borderL = 30;
	m_borderR = 45;
	m_borderTop = 80;
	m_borderBot = 80;
	m_historyW = 300;

	m_winHeight = m_numRows * m_cellSize + m_borderTop + m_borderBot;
	m_winWidth  = m_borderL + m_numCols * m_cellSize + m_borderR + m_historyW;
}

CRender::~CRender(void)
{
}

VOID CRender::CreateRenderContext(HWND hWnd)
{
	HGLRC hrc = NULL;
	PAINTSTRUCT ps;

	// Get a handle to the device context
	m_hdc = BeginPaint(hWnd, &ps);
	// Setup pixel format for the device context
	SetupPixelFormat(m_hdc);
	// Create a rendering context associated to the device context
	hrc = wglCreateContext(m_hdc);
	// Make the rendering context current
	wglMakeCurrent(m_hdc, hrc);
}
		
// Setup pixel format for the device context
void CRender::SetupPixelFormat(HDC hdc)
{
    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        16, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    int index = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, index, &pfd);
}

void CRender::output(int x, int y, const char *string)
{
  int len, i;

  glRasterPos2f((GLfloat)x, (GLfloat)y);
  len = (int) strlen(string);
  for (i = 0; i < len; i++) {
    glutBitmapCharacter(m_font, string[i]);
  }
}


GLvoid CRender::draw_circle(const GLfloat radius,const GLuint num_vertex, const GLfloat x, const GLfloat y)
{
	GLfloat vertex[4]; 
	//GLfloat texcoord[2];

	const GLfloat delta_angle = (GLfloat)2.0*Pi/num_vertex;

	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D,texID);
	//glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	glBegin(GL_TRIANGLE_FAN);

	//draw the vertex at the center of the circle
	//texcoord[0] = 0.5;
	//texcoord[1] = 0.5;
	//glTexCoord2fv(texcoord);

	vertex[0] = x;
	vertex[1] = y;
	vertex[2] = 0.0;
	vertex[3] = 1.0;        
	glVertex4fv(vertex);

	for(GLuint i = 0; i < num_vertex ; i++)
	{
		//texcoord[0] = (cos(delta_angle*i) + 1.0)*0.5;
		//texcoord[1] = (sin(delta_angle*i) + 1.0)*0.5;
		//glTexCoord2fv(texcoord);

		vertex[0] = cos(delta_angle*i) * radius + x;
		vertex[1] = sin(delta_angle*i) * radius + y;
		vertex[2] = 0.0;
		vertex[3] = 1.0;
		glVertex4fv(vertex);
	}

	//texcoord[0] = (1.0 + 1.0)*0.5;
	//texcoord[1] = (0.0 + 1.0)*0.5;
	//glTexCoord2fv(texcoord);

	vertex[0] = (GLfloat)(1.0 * radius + x);
	vertex[1] = (GLfloat)(0.0 * radius + y);
	vertex[2] = 0.0;
	vertex[3] = 1.0;
	glVertex4fv(vertex);
	glEnd();

	//glDisable(GL_TEXTURE_2D);
}



// Do OpenGL rendering
//                y
//                ^
//                |
//                |
//                |
//                |
//                +-----------> x
void CRender::RenderIt()
{
	int i;
	std::string s;

	//std::cout << "Enter Rendering.\n" << std::flush;
	
	// Reset the back buffer
    glClearColor(1.0, 1.0, 1.0, 1.0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, m_winWidth, m_winHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, m_winWidth, 0.0, m_winHeight);

    // Drawing - on the back buffer

	EnterCriticalSection( &g_csCheckerList );
	std::list<checker*>::iterator itr;
	itr = m_pThreadAI->m_listCheckers.begin();
	while (itr != m_pThreadAI->m_listCheckers.end()) {
		glColor3ub(((*itr)->rgb[0]), ((*itr)->rgb[1]), ((*itr)->rgb[2]));
		draw_circle((GLfloat)m_cellSize/2.0f, 48, (GLfloat)(*itr)->x, (GLfloat)(*itr)->y);
		itr++;
	}
	LeaveCriticalSection( &g_csCheckerList );
	
	glBegin(GL_LINES);
	glColor3b(0, 0, 0);
	// this left most vertical line starts 30 from left edge, 80 from bottom
	for (i=0;i<=m_numCols;i++) {
		glVertex2f((GLfloat)(m_borderL+i*m_cellSize), (GLfloat)m_borderBot);
		glVertex2f((GLfloat)(m_borderL+i*m_cellSize), (GLfloat)(m_borderBot + m_numRows*m_cellSize));
	}

	// this top most horizontal line starts 30 from left edge, 400 from bottom
	for (i=0;i<=m_numRows;i++) {
		glVertex2f((GLfloat)m_borderL, (GLfloat)(m_borderBot+i*m_cellSize));
		glVertex2f((GLfloat)(m_borderL+m_numCols*m_cellSize), (GLfloat)(m_borderBot+i*m_cellSize));
	}
	glEnd();

	for (i=1;i<=m_numCols;i++) {
		s = format("%c", 'a'+i-1);
		output(m_borderL+i*m_cellSize-m_cellSize/2, m_borderBot-18, s.c_str());
	}

	// this top most horizontal line starts 30 from left edge, 400 from bottom
	for (i=1;i<=m_numRows;i++) {
		s = format("%d", i);
		output(m_borderL/2, m_borderBot+i*m_cellSize-m_cellSize/2, s.c_str());
	}

	glColor3b(0, 0, 0);
	m_pThreadAI->updateMoveTime();
	output(m_borderL+4*m_cellSize, m_borderBot/2-0, "Elapsed Time: ");
	s = format("%d", m_pThreadAI->getMoveTime());
	output(m_borderL+4*m_cellSize+80, m_borderBot/2-0, s.c_str());
	
	output(m_borderL+4*m_cellSize, m_borderBot/2-15, "Evaluations: ");
	s = format("%d", m_pThreadAI->getNumEvals());
	output(m_borderL+4*m_cellSize+80, m_borderBot/2-15, s.c_str());

	output(m_borderL+4*m_cellSize, m_borderBot/2-27, "Depth: ");
	s = format("%d", m_pThreadAI->getDepthLimit());
	output(m_borderL+4*m_cellSize+80, m_borderBot/2-27, s.c_str());

	glColor3b(0, 0, 0);
	output(m_borderL, m_borderBot/2-15, m_pThreadAI->getGameState().c_str());

	glColor3ub((m_pThreadAI->getPlayerTurn()->color[0]), (m_pThreadAI->getPlayerTurn()->color[1]), (m_pThreadAI->getPlayerTurn()->color[2]));
	output(m_borderL+2*m_cellSize, m_borderBot/2-15, m_pThreadAI->getStatus().c_str());

	glColor3b(0, 0, 0);
	int left;
	int line;
	
	left = m_borderL+m_numCols*m_cellSize+m_borderR;
	line = m_borderBot+m_numRows*m_cellSize;
	output(left, line, "Game History");
	std::list<Move*>::iterator litr;
	EnterCriticalSection( &g_csMoveList );
	litr = m_pThreadAI->m_listMoves.begin();
	i = 1;
	while (litr != m_pThreadAI->m_listMoves.end()) {
		if ((*litr)->color == RED) {
			glColor3ub(255, 0, 0);
		}
		else {
			glColor3ub(0, 0, 0);
		}
		output(left, line-12*i, (*litr)->comment.c_str());
		++litr;
		i++;
	}
	LeaveCriticalSection( &g_csMoveList );

	glFlush();

    // Swap the back buffer with the front buffer
    SwapBuffers(m_hdc);

	//std::cout << "Finished Rendering.\n" << std::flush;
}




