// Copyright © 2011 Steve Coward

#include "StdAfx.h"
#include <queue>
#include "Connect3AIThread.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>

extern CRITICAL_SECTION g_csCheckerList;
extern CRITICAL_SECTION g_csMoveList;

CConnect3AIThread::CConnect3AIThread(int numRows, int numCols, int numCons)
{
#ifdef _DEBUG
	//t.disable(true);
	setTimeOut(10); // in seconds
#else
	setTimeOut(10); // in seconds
#endif // _DEBUG
#ifdef _DEBUG
	m_bGraphicDebug = false;
#endif // _DEBUG
	m_bTimeOutOnDepthLimit = false;
	m_bExit = false;
	m_p1 = new player();
	m_p2 = new player();
	m_p1->human = true;
	m_p1->_color = RED;
	m_p1->color[0] = 255;
	m_p1->color[1] = 0;
	m_p1->color[2] = 0;
	m_p2->human = false;
	m_p2->_color = BLACK;
	m_p2->color[0] = 0;
	m_p2->color[1] = 0;
	m_p2->color[2] = 0;
	setNumRows(numRows);
	setNumCols(numCols);
	setNumConnections(numCons);
	setWinMask();

	m_vBoard.resize( getNumRows() , std::vector<int>( getNumCols() , 0 ) );

	GameReset();
}

CConnect3AIThread::CConnect3AIThread(int numRows, int numCols, int numCons, std::wstring testFile)
{
#ifdef _DEBUG
	//t.disable(true);
	setTimeOut(10); // in seconds
#else
	setTimeOut(10); // in seconds
#endif // _DEBUG
#ifdef _DEBUG
	m_bGraphicDebug = false;
#endif // _DEBUG
	m_bTimeOutOnDepthLimit = false;
	m_bExit = false;
	m_p1 = new player();
	m_p2 = new player();
	m_p1->human = true;
	m_p1->_color = RED;
	m_p1->color[0] = 255;
	m_p1->color[1] = 0;
	m_p1->color[2] = 0;
	m_p2->human = false;
	m_p2->_color = BLACK;
	m_p2->color[0] = 0;
	m_p2->color[1] = 0;
	m_p2->color[2] = 0;
	setNumRows(numRows);
	setNumCols(numCols);
	setNumConnections(numCons);
	setWinMask();

	m_vBoard.resize( getNumRows() , std::vector<int>( getNumCols() , 0 ) );

	GameReset();

	//setGameState(GAME_SETUP);

	//std::string line;
	//std::ifstream myfile (testFile);
	//int lineno = 0;
	//int i, j;
	//if (myfile.is_open())
	//{
	//	while ( myfile.good() )
	//	{
	//		for (i=0; i<numRows; i++) {
	//			std::getline(myfile, line);
	//			for (j=0; j<numCols; j++) {
	//				if (m_p1->_color == line[j]-'0') {
	//					DoMove(line[j]-'0', m_p1, true);
	//				}
	//				else if (m_p2->_color == line[j]-'0') {
	//					DoMove(line[j]-'0', m_p2, true);
	//				}
	//			}
	//		}
	//		
	//		std::cout << line << std::endl;
	//		lineno++;
	//	}
	//	myfile.close();
	//}
}

CConnect3AIThread::~CConnect3AIThread(void)
{
	GameReset();
	if (m_p1 != NULL) {
		delete m_p1;
		m_p1 = NULL;
	}
	if (m_p2 != NULL) {
		delete m_p2;
		m_p2 = NULL;
	}
}

void CConnect3AIThread::GameReset()
{
	int r, c;

	m_bSymmetryBroken = false;
	m_bBoardSymmetric = true;
	m_numMismatchPairs = 0;

	t.start();

	setNumSlots();
	setForceMove(false);
	m_bGameSetup = false;
	m_bAnalysisMode = false;
	m_bHitDepthLimit = false;
	m_bTimedOut = false;
	m_plyNum = 0;
	clrNumEvals();
	m_pTurn = m_p1;
	m_pWait = m_p2;
	setGameState(GAME_STARTED_PLAYER1_TO_MOVE);
	setStatus("It's your turn");

	for (r=0;r<getNumRows();r++) {
		for (c=0;c<getNumCols();c++) {
			m_vBoard.at(r).at(c) = EMPTY;
		}
	}

	EnterCriticalSection( &g_csMoveList );
	std::list<Move*>::iterator litr;
	while (!m_listMoves.empty()) {
		litr = m_listMoves.begin();
		Move* m = (*litr);
		delete m;
		m_listMoves.erase(litr);
	}
	LeaveCriticalSection( &g_csMoveList );

	EnterCriticalSection( &g_csCheckerList );
	std::list<checker*>::iterator citr;
	while (!m_listCheckers.empty()) {
		citr = m_listCheckers.begin();
		checker* ch = (*citr);
		delete ch;
		m_listCheckers.erase(citr);
	}
	LeaveCriticalSection( &g_csCheckerList );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Translate point from window coordinates (x,y) to game (row,column).
//  Window coordinates: x increases to right, y, increases downwards.
//  Game coordinates: row increases upwards starting at 0 (internally)
//                    col increases to right starting at 0 (internally).
// Verify legality of move.
// Return true if the move is legal.
// Return (row,column) pair of the move (zero-based).
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool CConnect3AIThread::CheckLegality(const Point& p, int& r, int& c)
{
	// must account for height of menu and title bar here
	// since (x,y) = (0,0) just below menu bar and to right of side frame
	c = (p.x + getCellSize() - m_boardPosX)/getCellSize() - 1;
	r = getNumRows() + 1 - ((p.y + getCellSize() - 25)/getCellSize()) - 1;
	if ((r >= 0) && (r < getNumRows())) {
		if ((c >= 0) && (c < getNumCols())) {
			return(
				(m_vBoard[r][c] == EMPTY) &&
					((r == 0) || (m_vBoard[r-1][c] != EMPTY)));
		}
	}
	return(false);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Verify legality of move.
// Update game move history.
// Call DoMove().
// If enabled, calculate analysis of move (can be very time-consuming.)
// Return true if move was legal.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Move* CConnect3AIThread::DoHumanMove(const Point& p)
{
	int r, c;
	Move* m = NULL;

	if (CheckLegality(p, r, c)) {
		DoMove(c, m_pTurn, true);
		m = m_listMoves.back();
		if (m_bAnalysisMode) {
			DoMiniMax(true);
		}
		else if (m_bGameSetup) {
			// Output display row numbering starts at 1. 
			m->comment =
				format("%2d. %c%d Game setup",
				m_plyNum, c+'a', r+1);
		}
		else {
			// Output display row numbering starts at 1. 
			m->comment =
				format("%2d. %c%d Human move (%d s)",
				m_plyNum, c+'a', r+1, t.elapsedTime());
		}
	}

	return(m);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Run game loop.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CConnect3AIThread::Connect3()
{
	while (!m_bExit) {
		if (m_gameState == GAME_RESET) {
			GameReset();
		}

		//::Sleep(1000);
		if (m_gameState & (GAME_STARTED | GAME_OVER)) {
			Move* m = NULL;
			if (m_pTurn->human || (m_gameState & GAME_OVER)) {
				setStatus("It's your turn");
				while (!m_qCmds.empty()) {
					Cmd* c = m_qCmds.front();
					m_qCmds.pop();
					switch (c->cmd) {
						case NEW_GAME:
							{
								// New game requested
								setGameState(GAME_RESET);
								break;
							}
						case ANALYSIS:
							{
								// Toggle human move analysis mode
								//   - Can be time consuming
								if (m_bAnalysisMode) {
									m_bAnalysisMode = false;
								}
								else {
									m_bAnalysisMode = true;
								}
								break;
							}
						case DISABLETIMER:
							{
								// Toggle timer
								t.setDisable(!t.getDisable());
								break;
							}
						case UNDO:
							{
								// Undo last two moves
								UndoMove();
								UndoMove();

								if (m_gameState & GAME_OVER) {
									if (m_pTurn->human) {
										setGameState(GAME_STARTED_PLAYER1_TO_MOVE);
									}
									else {
										setGameState(GAME_STARTED_PLAYER2_TO_MOVE);
									}
								}

								break;
							}
						case SETUP_GAME:
							{
								// Toggle game setup mode
								setGameState(GAME_SETUP);
								m_bGameSetup = true;
								break;
							}
						case HINT:
							{
								if (m_gameState & GAME_STARTED) {
									// Hint requested
									m = DoMiniMax(true);
								}
								break;
							}
						case SWAP_SIDES:
							{
								if (m_gameState & GAME_STARTED) {
									// Swap side requested
									int tempc;
									if (m_pTurn == m_p1) {
										m_pTurn = m_p2;
										m_pWait = m_p1;
									}
									else {
										m_pTurn = m_p1;
										m_pWait = m_p2;
									}

									tempc = m_p1->_color;
									m_p1->_color = m_p2->_color;
									m_p2->_color = tempc;

									tempc = m_p1->color[0];
									m_p1->color[0] = m_p2->color[0];
									m_p2->color[0] = tempc;
									tempc = m_p1->color[1];
									m_p1->color[1] = m_p2->color[1];
									m_p2->color[1] = tempc;
									tempc = m_p1->color[2];
									m_p1->color[2] = m_p2->color[2];
									m_p2->color[2] = tempc;
								}

								break;
							}
						default:
							{
								if (m_gameState & GAME_STARTED) {
									m = DoHumanMove(c->p);
								}
							}
					}
					delete c;
				}
			}
			else  {
				// computer's turn
				if (m_gameState & GAME_STARTED) {
					// do computer AI
					setStatus("It's the computer's turn");
					m = DoMiniMax(false);
					std::cout << "Computer has moved.\n";
				}
			}

			if (m != NULL) {
				if (IncEvaluateWin(m->row, m->col)) {
					if (m_pTurn->human) {
						setGameState(GAME_OVER_HUMAN_WINS);
						setStatus("You Win");
					}
					else {
						setGameState(GAME_OVER_COMPUTER_WINS);
						setStatus("Computer Wins");
					}
				}
				else if ((m_gameState != GAME_OVER_DRAW) && BoardFull(&m_vBoard))
				{
					setGameState(GAME_OVER_DRAW);
					setStatus("Drawn Game");
				}
			}
		}
		else if (m_gameState == GAME_SETUP) {
			while (!m_qCmds.empty()) {
				Cmd* c = m_qCmds.front();
				m_qCmds.pop();
				switch (c->cmd) {
					case SETUP_GAME:
						{
							// Toggle game setup mode
							if (m_bGameSetup) {
								clrNumEvals();
								if (m_pTurn == m_p1) {
									setGameState(GAME_STARTED_PLAYER1_TO_MOVE);
								}
								else {
									setGameState(GAME_STARTED_PLAYER2_TO_MOVE);
								}
								m_bGameSetup = false;
							}
							break;
						}
					case UNDO:
						{
							// Undo last move
							UndoMove();
							break;
						}
					default:
						{
							Move* m;
							if ((m = DoHumanMove(c->p)) != NULL) {
								if (IncEvaluateWin(m->row, m->col)) {
									// Board contains winning combination.
									// Disallow last placement.
									UndoMove();
								}
								else if (BoardFull(&m_vBoard)) {
									// Board is completely full.
									UndoMove();
								}
							}
						}
				}
			}
		}
		else {
			while (!m_qCmds.empty()) {
				Cmd* c = m_qCmds.front();
				m_qCmds.pop();
				if (c->cmd == NEW_GAME) {
					setGameState(GAME_RESET);
				}
			}
		}
	}
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Determine row that checker will be placed in.
// Set that board position to the appropriate color.
// Increment ply number.
// Push a move onto the move list for move history.
// Push a checker onto the checker list for display.
// Rotate turns.
// Return row that the checker was placed in.
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int CConnect3AIThread::DoMove(const int col, player* p, bool bPlace)
{
	int row;
	
	assert((p == m_pTurn) || (m_gameState == GAME_SETUP));

	for (row=0;row<getNumRows();row++) {
		if (m_vBoard.at(row).at(col) == EMPTY) {
			break;
		}
	}
	assert(row < getNumRows());
	if (row == getNumRows()) {
		return(-1);
	}
	else if (!bPlace) {
		return(row);
	}

	// Add the move to the move list
	Move* m = new Move();
	m->col = col;
	m->row = row;
	m->moveNumber = m_plyNum;
	m->color = p->_color;
	EnterCriticalSection( &g_csMoveList );
	m_listMoves.push_back(m);
	LeaveCriticalSection( &g_csMoveList );

	// Update game board
	m_vBoard.at(row).at(col) = p->_color;

	// Update symmetry state
	// Issue this is probably not worth it.
	// Issue This needs to be maintained recursively with board state
	if (!m_bSymmetryBroken) {
		if (m_numCols & 1) {
			// odd number of columns
			if (col == m_numCols/2) {
				// Placed checker in middle column
				// No change in symmetry state
			}
			else {
				// Placed on left or right
				switch (m_vBoard[row][col] - m_vBoard[row][getNumCols()-col-1]) {
					case 0:
					case 2:
					case -2:
						m_bBoardSymmetric = (m_numMismatchPairs == 0);
						m_numMismatchPairs--;
						break;
					case 1:
					case -1:
						m_bSymmetryBroken = true;
						m_numMismatchPairs--;
						break;
					default:
						m_bBoardSymmetric = false;
						m_numMismatchPairs++;
						break;
				}
			}
		}
		else {
		}
	}

	// place a checker in the display view
	checker* ch = new checker();
	ch->rgb[0] = 0;
	ch->rgb[1] = 0;
	ch->rgb[2] = 0;
	if (p->_color == RED) {
		ch->rgb[0] = 255;
	}
	// opengl coordinates, y increasing upwards
	ch->x = (col) * getCellSize() + getBoardPosX() + getCellSize()/2;
	ch->y = (row) * getCellSize() + getBoardPosY() + getCellSize()/2;
	EnterCriticalSection( &g_csCheckerList );
	m_listCheckers.push_front(ch);
	LeaveCriticalSection( &g_csCheckerList );

	// rotate turns
	if (p == m_p1) {
		m_pTurn = m_p2;
		m_pWait = m_p1;
	}
	else {
		m_pTurn = m_p1;
		m_pWait = m_p2;
	}

	m_plyNum++;

	return(row);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Remove checker from board. Mark spot as EMPTY.
// Decrement ply number.
// Pop a checker from the checker list for display.
// Pop a move onto the move list for move history.
// Rotate turns.
// Issue: Affects symmetry.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CConnect3AIThread::UndoMove()
{
	checker* ch;
	Move* m;

	assert(m_listCheckers.size() == m_listMoves.size());
	assert(m_listCheckers.size() == m_plyNum);
	
	if (!m_listCheckers.empty()) {
		m_plyNum--;

		if (m_pTurn == m_p1) {
			m_pTurn = m_p2;
			m_pWait = m_p1;
		}
		else {
			m_pTurn = m_p1;
			m_pWait = m_p2;
		}

		EnterCriticalSection( &g_csCheckerList );
		ch = m_listCheckers.front();
		m_listCheckers.pop_front();
		delete ch;
		LeaveCriticalSection( &g_csCheckerList );

		EnterCriticalSection( &g_csMoveList );
		m = m_listMoves.back();
		m_listMoves.pop_back();
		LeaveCriticalSection( &g_csMoveList );

		m_vBoard.at(m->row).at(m->col) = EMPTY;

		delete m;
	}
}

int CConnect3AIThread::MiniMaxHuman(
	int depth,
	int numEmptySlots,
	int& movecol,
	int prow,
	int pcol,
	int alpha,
	int beta)
{
	int row, col, mc, mcc;
	int v, minV;

	// The computer (max) player just made a move,
	// so evaluate that move here
	if ((v = IncEvaluateWin(prow, pcol)) > 0) {
		return(numEmptySlots);
	}
	else if (depth >= getDepthLimit()) {
		m_bHitDepthLimit = true;
		if ((v = IncEvaluateBlock(prow, pcol)) > 0) {
			return(numEmptySlots-1);
		}
		else {
			return(alpha);
		}
	}
	else if (getForceMove()) {
		m_bTimedOut = true;
		return(alpha);
	}
	minV = getNumSlots()+1;
	for (col=0;col<getNumCols();col++) {
		mc = col;
		if (ColOpen(m_vBoard, col, &row)) {
#ifdef _DEBUG
			if (m_bGraphicDebug) {
				// this gives visual display of search but is very slow
				row = DoMove(col, m_pTurn, true);
				//prettyPrintBoard(&vBoard, getNumSlots()-depth+1);
				v = MiniMaxComputer(
					depth+1,
					numEmptySlots-1,
					mcc,
					row,
					col,
					alpha,
					beta
					);
				UndoMove();
				::Sleep(200);
			}
			else {
				m_vBoard.at(row).at(col) = m_pWait->_color;
				v = MiniMaxComputer(
					depth+1,
					numEmptySlots-1,
					mcc,
					row,
					col,
					alpha,
					beta
					);
				m_vBoard.at(row).at(col) = EMPTY;
			}
#else
			// Issue Here is where recursive symmetry update must be performed
			m_vBoard.at(row).at(col) = m_pWait->_color;
			v = MiniMaxComputer(
				depth+1,
				numEmptySlots-1,
				&mcc,
				row,
				col,
				alpha,
				beta
			);
			m_vBoard.at(row).at(col) = EMPTY;
#endif
			if (v <= minV) {
				minV = v;
				movecol = mc;
			}
			if (v < beta) {
				beta = v;
			}
			if (alpha >= beta) {
				return(beta);
			}
			if (t.isTimeout(getTimeOut())) {
				m_bTimedOut = true;
				return(beta);
			}
		}
	}

	if (minV == (getNumSlots()+1)) {
		// DRAW
		return(0);
	}
	else {
		return(minV);
	}
}

int CConnect3AIThread::MiniMaxComputer(
	int depth, // current recursion depth
	int numEmptySlots, // the number of empty slots remaining
	int& movecol, // return the chosen column here
	int prow, // previous move's row
	int pcol, // previous move's column
	int alpha,
	int beta)
{
	int row, col, mc, mcc;
	int v, maxV;

	// The human (min) player just made a move,
	// so evaluate that move here
	if ((v = IncEvaluateWin(prow, pcol)) > 0) {
		return(-numEmptySlots);
	}
	else if (depth >= getDepthLimit()) {
		m_bHitDepthLimit = true;
		if ((v = IncEvaluateBlock(prow, pcol)) > 0) {
			return(-numEmptySlots+1);
		}
		else {
			return(alpha);
		}
	}
	else if (getForceMove()) {
		m_bTimedOut = true;
		return(alpha);
	}
	maxV = -(getNumSlots()+1);

	for (col=0;col<getNumCols();col++) {
		mc = col;
		if (ColOpen(m_vBoard, col, &row)) {
#ifdef _DEBUG
			if (m_bGraphicDebug) {
				// this gives visual display of search but is very slow
				row = DoMove(col, m_pTurn, true);
				//prettyPrintBoard(&vBoard, getNumSlots()-depth+1);
				v = MiniMaxHuman(
					depth+1,
					numEmptySlots-1,
					mcc,
					row,
					col,
					alpha,
					beta
					);
				UndoMove();
				::Sleep(200);
			}
			else {
				m_vBoard.at(row).at(col) = m_pTurn->_color;
				v = MiniMaxHuman(
					depth+1,
					numEmptySlots-1,
					mcc,
					row,
					col,
					alpha,
					beta
				);
				m_vBoard.at(row).at(col) = EMPTY;
			}
#else
			// Issue Here is where recursive symmetry update must be performed
			m_vBoard.at(row).at(col) = m_pTurn->_color;
			v = MiniMaxHuman(
				depth+1,
				numEmptySlots-1,
				&mcc,
				row,
				col,
				alpha,
				beta
			);
			m_vBoard.at(row).at(col) = EMPTY;
#endif
			if (v > maxV) {
				maxV = v;
				movecol = mc;
			}
			if (v > alpha) {
				alpha = v;
			}
			if (alpha >= beta) {
				return(alpha);
			}
			if (t.isTimeout(getTimeOut())) {
				m_bTimedOut = true;
				return(alpha);
			}
		}
	}

	if (maxV == -(getNumSlots()+1)) {
		// DRAW
		return(0);
	}
	else {
		return(maxV);
	}
}




Move* CConnect3AIThread::DoMiniMax(bool bHintMode)
{
	int v;
	int col;
	int bestV;
	int bestCol = -1;
	Move* m = NULL;

	// Reset timer to enable time out of AI
	t.start();
	clrNumEvals();

#ifdef _DEBUG
	// Disable iterative deepening for debug mode
	//setDepthLimit(getNumSlots());
	setDepthLimit(1);
#else 
	// Always use iterative deepening for release mode
	setDepthLimit(1);
#endif // _DEBUG

	setDepth(0);
	setForceMove(false);
	m_bTimedOut = false;
	m_bHitDepthLimit = false;
	while (!m_bTimedOut && !getForceMove()) {
		v = MiniMaxComputer(
			getDepth(), // max recursive depth for this iteration
			getNumSlots()-m_plyNum+1, // starting valuation
			col, // best column found
			0, // row of last placed checker (for incremental evaluation)
			0, // col of last placed checker (for incremental evaluation)
			-(getNumSlots()-m_plyNum), // alpha
			(getNumSlots()-m_plyNum) // beta
		);
		std::cout << "at depth " << getDepthLimit() << " Best column is " << col << " with value " << v << ".\n" << std::flush;

		if (m_bHitDepthLimit) {
			// this depth iteration completed
			// so save results
			bestV = v;
			bestCol = col;
			m_bHitDepthLimit = false;
		}
		else if (!m_bTimedOut) {
			// search finished,
			// depth iteration was not completed, and
			// did not time out
			// so save results and stop iterating
			bestV = v;
			bestCol = col;
			break;
		}
//#ifndef _DEBUG
	incDepthLimit();
//#endif // _DEBUG
	}

	int row;
	if (bestCol < 0) {
		// Whoops - search must have been preempted before completion
		// Find some move
		for (col = 0; col<getNumCols();col++) {
			if (ColOpen(m_vBoard, col, &row)) {
				bestCol = col;
				break;
			}
		}
	}
	assert(bestCol >= 0);

	// Now actually perform the move that was just calculated
	row = DoMove(bestCol, m_pTurn, !bHintMode);
	if (!m_listMoves.empty()) {
		m = m_listMoves.back();
	}

	// Form an intelligent comment on the move results
	if (m != NULL) {
		m->comment = FormatComment(bestCol, bestV, row, bHintMode);
	}

	// Reset timer for next player
	t.start();

	return(m);
}

// Form an intelligent comment on the move results
std::string CConnect3AIThread::FormatComment(int bestCol, int bestV, int row, bool bHintMode) {
	if (bHintMode) {
		std::cout << "The computer recommends column ";
		std::cout << bestCol << std::endl << std::flush;
		if (bestV == 0) {
			if ((getNumSlots()-m_plyNum+1) == 0) {
				return(
					format("%2d. %c%d Drawn Game (%d s)",
					m_plyNum,
					bestCol+'a',
					row+1,
					t.elapsedTime()
					)
					);
			}
			else {
				return(
					format("%2d. %c%d Draw in %d moves (%d s)",
					m_plyNum,
					bestCol+'a',
					row+1,
					getNumSlots()-m_plyNum,
					t.elapsedTime()
					)
					);
			}
		}
		else if (bestV > 0) {
			if ((getNumSlots()-m_plyNum-bestV+1) == 0) {
				return(
					format("%2d. %c%d Computer wins! (%d s)",
					m_plyNum,
					bestCol+'a',
					row+1,
					t.elapsedTime()
					)
					);
			}
			else {
				return(
					format("%2d. %c%d Computer wins in %d moves (%d s)",
					m_plyNum,
					bestCol+'a',
					row+1,
					getNumSlots()-m_plyNum-bestV+1,
					t.elapsedTime()
					)
					);
			}
		}
	}
	else {
		if (m_bTimedOut|m_bHitDepthLimit) {
			return(
				format("%2d. %c%d Search timed out (%d s/%d/%d)",
				m_plyNum,
				bestCol+'a',
				row+1,
				t.elapsedTime(),
				getNumEvals(),
				getDepthLimit()
				)
				);
		}
		else if (bestV == 0) {
			if ((getNumSlots()-m_plyNum) == 0) {
				return(
					format("%2d. %c%d Drawn Game (%d s/%d/%d)",
					m_plyNum,
					bestCol+'a',
					row+1,
					t.elapsedTime(),
					getNumEvals(),
					getDepthLimit()
					)
					);
			}
			else {
				return(
					format("%2d. %c%d Draw in %d moves (%d s/%d/%d)",
					m_plyNum,
					bestCol+'a',
					row+1,
					getNumSlots()-m_plyNum,
					t.elapsedTime(),
					getNumEvals(),
					getDepthLimit()
					)
					);
			}
		}
		else if (bestV > 0) {
			if ((getNumSlots()-m_plyNum-bestV+1) == 0) {
				return(
					format("%2d. %c%d Computer wins! (%d s/%d/%d)",
					m_plyNum,
					bestCol+'a',
					row+1,
					t.elapsedTime(),
					getNumEvals(),
					getDepthLimit()
					)
					);
			}
			else {
				return(
					format("%2d. %c%d Computer wins in %d moves (%d s/%d/%d)",
					m_plyNum,
					bestCol+'a',
					row+1,
					getNumSlots()-m_plyNum-bestV+1,
					t.elapsedTime(),
					getNumEvals(),
					getDepthLimit()
					)
					);
			}
		}
		else if (bestV < 0) {
			if ((getNumSlots()-m_plyNum-bestV) == 0) {
				return(
					format("%2d. %c%d Human wins! (%d s/%d/%d)",
					m_plyNum,
					bestCol+'a',
					row+1,
					t.elapsedTime(),
					getNumEvals(),
					getDepthLimit()
					)
					);
			}
			else {
				return(
					format("%2d. %c%d Human wins in %d moves (%d s/%d/%d)",
					m_plyNum,
					bestCol+'a',
					row+1,
					getNumSlots()-m_plyNum+bestV+1,
					t.elapsedTime(),
					getNumEvals(),
					getDepthLimit()
					)
				);
			}
		}
	}

	return("No comment");
}

bool CConnect3AIThread::ColOpen(intv2d& board, int col, int* row) const
{
	int r;

	for (r=0;r<getNumRows();r++) {
		if (board[r][col] == EMPTY) {
			*row = r;
			return(true);
		}
	}

	return(false);

}

// check only top row for an empty slot
bool CConnect3AIThread::BoardFull(const intv2d* board) const
{
	int r, c;

	r = getNumRows()-1;
	for (c=0;c<getNumCols();c++) {
		if (m_vBoard.at(r).at(c) == EMPTY) {
			return(false);
		}
	}

	return(true);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Incremental Evaluate.
// Consider effect of placing the last checker at (row,col) only.
// Assume there are no winning combinations not involving this checker.
// Return 1 if a winning combination is found, else return 0.
//
// Assume that magnitude of returned value is not important -
// that is to say that it will only be compared to 0.
//
// To enable reasonable moves to be made when the search does not complete,
// intelligence must be added to this evaluation function.  This intelligence
// must be able to favor one board position over another based on board state
// rather than on completion of a full search.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int CConnect3AIThread::IncEvaluateWin(int row, int col)
{
	int r, c;
	int v = 0;
	int minCol, maxCol, minRow, maxRow;
	int minupstart, mindnstart;
	int rv, cv, uv, dv;
	int match;
	int b;
	int m = getWinMask();

	incNumEvals();

	match = m_vBoard[row][col];

	minCol = max(col-m_numCons+1,0);
	maxCol = min(col+m_numCons-1,m_numCols-1);
	minRow = max(row-m_numCons+1,0);
	maxRow = min(row+m_numCons-1,m_numRows-1);
	minupstart = min(row-minRow, col-minCol);
	mindnstart = min(maxRow-row, col-minCol);

	// check if row contains connection
	rv = 0;
	for (c=minCol;c<=maxCol;c++) {
		b = m_vBoard[row][c];
		rv = (rv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}
	while (rv >= m) {
		if ((rv & m) == m) return(1);
		rv = rv >> 1;
	}

	// check if column contains connection
	cv = 0;
	for (r=row;r>=minRow;r--) {
		b = m_vBoard[r][col];
		cv = (cv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}
	while (cv >= m) {
		if ((cv & m) == m) return(1);
		cv = cv >> 1;
	}

	// check if up diagonal contains connection
	uv = 0;
	for (r=row-minupstart,c=col-minupstart;(r<=maxRow)&&(c<=maxCol);r++,c++) {
		b = m_vBoard[r][c];
		uv = (uv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}
	while (uv >= m) {
		if ((uv & m) == m) return(1);
		uv = uv >> 1;
	}

	// check if down diagonal contains connection
	dv = 0;
	for (r=row+mindnstart,c=col-mindnstart;(r>=minRow)&&(c<=maxCol);r--,c++) {
		b = m_vBoard[r][c];
		dv = (dv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}
	while (dv >= m) {
		if ((dv & m) == m) return(1);
		dv = dv >> 1;
	}

	return(0);
}

int CConnect3AIThread::IncEvaluateBlock(int row, int col)
{
	int r, c;
	int v = 0;
	int minCol, maxCol, minRow, maxRow;
	int minupstart, mindnstart;
	int rv, cv, uv, dv;
	int match;
	int b;
	int m = getWinMask();
	int block = 0;

	incNumEvals();

	// switch slot to opposite color
	match = m_vBoard[row][col] ^ 1;
	m_vBoard[row][col] = match;

	minCol = max(col-m_numCons+1,0);
	maxCol = min(col+m_numCons-1,m_numCols-1);
	minRow = max(row-m_numCons+1,0);
	maxRow = min(row+m_numCons-1,m_numRows-1);
	minupstart = min(row-minRow, col-minCol);
	mindnstart = min(maxRow-row, col-minCol);

	// check if row contains connection
	rv = 0;
	for (c=minCol;c<=maxCol;c++) {
		b = m_vBoard[row][c];
		rv = (rv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}
	while (rv >= m) {
		if ((rv & m) == m) { block = 1; goto cleanup; }
		rv = rv >> 1;
	}

	// check if column contains connection
	cv = 0;
	for (r=row;r>=minRow;r--) {
		b = m_vBoard[r][col];
		cv = (cv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}
	while (cv >= m) {
		if ((cv & m) == m) { block = 1; goto cleanup; }
		cv = cv >> 1;
	}

	// check if up diagonal contains connection
	uv = 0;
	for (r=row-minupstart,c=col-minupstart;(r<=maxRow)&&(c<=maxCol);r++,c++) {
		b = m_vBoard[r][c];
		uv = (uv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}
	while (uv >= m) {
		if ((uv & m) == m) { block = 1; goto cleanup; }
		uv = uv >> 1;
	}

	// check if down diagonal contains connection
	dv = 0;
	for (r=row+mindnstart,c=col-mindnstart;(r>=minRow)&&(c<=maxCol);r--,c++) {
		b = m_vBoard[r][c];
		dv = (dv << 1) | ((~(match ^ (b & 1)) & 1) & ~(b >> 1));
	}

	while (dv >= m) {
		if ((dv & m) == m) { block = 1; goto cleanup; }
		dv = dv >> 1;
	}

cleanup:
	// switch slot back to original color
	match = match ^ 1;
	m_vBoard[row][col] = match;

	return(block);
}

#ifdef _DEBUG
void CConnect3AIThread::prettyPrintBoard(intv2d* pvBoard, int boardNum)
{ 	int r, c;

	if (boardNum > getNumRows() * getNumCols()) {
		std::cout << "Depth too deep - no board printed.\n";
	}

	std::cout << "Board Number " << boardNum << std::endl;

	for (r=getNumRows()-1;r>=0;r--) {
		for (c=0;c<getNumCols();c++) {
			switch (pvBoard->at(r).at(c)) {
				case RED:
					std::cout << "X";
					break;
				case BLACK:
					std::cout << "O";
					break;
				case EMPTY:
					std::cout << "-";
					break;
			}
		}
		std::cout << std::endl << std::endl << std::flush;
	}
}
#endif // _DEBUG

void CConnect3AIThread::setStatus(std::string status) {
	m_status = status;
}

void CConnect3AIThread::setGameState(GameStates gameState) {
	m_gameState = gameState;
	switch (m_gameState) {
			case GAME_RESET:
				m_strGameState = "";
				break;
			case GAME_STARTED_PLAYER1_TO_MOVE:
				m_strGameState = "It is player1's turn";
				break;
			case GAME_STARTED_PLAYER2_TO_MOVE:
				m_strGameState = "It is player2's turn";
				break;
			case GAME_OVER_DRAW:
				m_strGameState = "Drawn game.";
				break;
			case GAME_OVER_HUMAN_WINS:
				m_strGameState = "Game over - human wins";
				break;
			case GAME_OVER_COMPUTER_WINS:
				m_strGameState = "Game over - computer wins";
				break;
			case GAME_SETUP:
				m_strGameState = "Set up game Position.";
				break;
	}

	std::cout << m_strGameState.c_str() << "\n";
}
