#pragma once

#include "gamedef.h"
#include "Timer.h"
#include <vector>
#include <queue>
#include <list>

typedef std::vector<std::vector<int> > intv2d;

class CConnect3AIThread
{
private:
	enum GameStates {
		GAME_RESET = 0x1,
		GAME_STARTED_PLAYER1_TO_MOVE = 0x5,
		GAME_STARTED_PLAYER2_TO_MOVE = 0x6,
		GAME_STARTED = 0x4, // Mask
		GAME_OVER = 0x8, // Mask
		GAME_OVER_DRAW = 0xb,
		GAME_OVER_HUMAN_WINS = 0x9,
		GAME_OVER_COMPUTER_WINS = 0xa,
		GAME_SETUP = 0x10
	};

	GameStates m_gameState;
	intv2d m_vBoard;

#ifdef _DEBUG
	bool m_bGraphicDebug;
#endif // _DEBUG
	bool m_bGameSetup;
	bool m_bTimedOut;
	bool m_bTimeOutOnDepthLimit;
	bool m_bHitDepthLimit;
	bool m_bAnalysisMode;

	int m_plyNum;
	
	// Symmetry maintenance variables
	bool m_bBoardSymmetric;
	bool m_bSymmetryBroken;
	int m_numMismatchPairs;

	player* m_p1;
	player* m_p2;
	player* m_pHuman;
	player* m_pComputer;
	player* m_pWait; // player whose turn it is not
private:
	player* m_pTurn; // player whose turn it is
public:
	player* getPlayerTurn() { return(m_pTurn); }

private:
	CTimer t;
	unsigned long m_timeoutSec;
public:
	void setTimeOut(unsigned long timeoutSec) { m_timeoutSec = timeoutSec; }
	unsigned long getTimeOut() {return(m_timeoutSec); }

public:
	std::list<checker*> m_listCheckers;
	std::list<Move*> m_listMoves;
	std::queue<Cmd*> m_qCmds;

public:
	CConnect3AIThread(int numRows, int numCols, int numCons);
	~CConnect3AIThread(void);

	// Thread entry
	void Connect3();

private:
	void GameReset();
	Move* DoMiniMax(bool bHintMode);
	int MiniMaxHuman(
		int depth,
		int numEmptySlots,
		int& movecol,
		int prow,
		int pcol,
		int alpha,
		int beta);
	int MiniMaxComputer(
		int depth,
		int numEmptySlots,
		int& movecol,
		int prow,
		int pcol,
		int alpha,
		int beta);
	Move* DoHumanMove(Point& p);
	int DoMove(int col, player* p, bool bPlace);
	void UndoMove();
	bool GameOver(intv2d* board);
	int IncEvaluateWin(int row, int col);
	int IncEvaluateBlock(int row, int col);
	bool ColOpen(intv2d& board, int col, int* row);
	bool CheckLegality(Point& p, int& r, int& c);
#ifdef _DEBUG
	void prettyPrintBoard(intv2d* pvBoard, int boardNum);
#endif // _DEBUG

private:	
	int m_threadID; // for message passing purposes
public:
	void setThreadID(int threadID) { m_threadID = threadID; }
	int getThreadID() const { return(m_threadID); }

private:	
	DWORD m_parentThreadID; // for messaging completion of this thread to parent thread
public:
	void setParentThreadID(DWORD parentThreadID) { m_parentThreadID = parentThreadID; }
	DWORD getParentThreadID() const { return(m_parentThreadID); }

private:
	HANDLE m_hThisThread; // handle to this thread (returned by _beginthreadex()), 
public:
	void setThreadHandle(const HANDLE hThisThread) { m_hThisThread = hThisThread; }
	HANDLE getThreadHandle() const { return( m_hThisThread); }

private:	
	bool m_bExit; // thread exit flag
public:
	void setExit(bool bExit) { m_bExit = bExit; }
	bool getExit() const { return(m_bExit); }
private:	
	bool m_bForceMove; 
public:
	void setForceMove(bool bForceMove) { m_bForceMove = bForceMove; }
	bool getForceMove() const { return(m_bForceMove); }

private:
	std::string m_status;
public:
	void setStatus(std::string status);
	std::string getStatus() { return(m_status); }

private:
	std::string m_strGameState;
public:
	void setGameState(GameStates gameState);
	std::string getGameState() { return(m_strGameState); }

private:
	int m_disWidth;
public:
	void setDisplayWidth(int disWidth) { m_disWidth = disWidth; }
	int getDisplayWidth() { return(m_disWidth); }
private:
	int m_disHeight;
public:
	void setDisplayHeight(int disHeight) { m_disHeight = disHeight; }
	int getDisplayHeight() { return(m_disHeight); }
private:
	int m_numRows;
public:
	void setNumRows(int numRows) { m_numRows = numRows; }
	int getNumRows() { return(m_numRows); }
private:
	int m_numCols;
public:
	void setNumCols(int numCols) { m_numCols = numCols; }
	int getNumCols() { return(m_numCols); }
private:
	int m_numSlots;
public:
	void setNumSlots() { m_numSlots = m_numCols * m_numRows; }
	int getNumSlots() { return(m_numSlots); }
private:
	int m_numCons;
public:
	void setNumConnections(int numCons) { m_numCons = numCons; }
	int getNumConnections() { return(m_numCons); }
private:
	int m_winMask;
public:
	void setWinMask() {
		m_winMask = 0;
		for (int r=0;r<getNumConnections();r++) {
			m_winMask |= (m_winMask << 1) | 1;
		}
	};
	int getWinMask() { return(m_winMask); }
private:
	int m_boardPosX;
public:
	void setBoardPosX(int boardPosX) { m_boardPosX = boardPosX; }
	int getBoardPosX() { return(m_boardPosX); }
private:
	int m_boardPosY;
public:
	void setBoardPosY(int boardPosY) { m_boardPosY = boardPosY; }
	int getBoardPosY() { return(m_boardPosY); }
private:
	int m_cellsize;	// in pixels
public:
	void setCellSize(int cellsize) { m_cellsize = cellsize; }
	int getCellSize() { return(m_cellsize); }
private:
	unsigned long m_moveduration;	// in seconds
public:
	void updateMoveTime() { m_moveduration = t.elapsedTime(); }
	unsigned long getMoveTime() { return(m_moveduration); }
private:
	int m_numEvals;
public:
	void clrNumEvals() { m_numEvals = 0; }
	void incNumEvals() { m_numEvals++; }
	int getNumEvals() { return(m_numEvals); }
private:
	int m_depth;	// search depth of last completed search
public:
	void setDepth(int depth) { m_depth = depth; }
	void incDepth() { m_depth++; }
	int getDepth() { return(m_depth); }
private:
	int m_depthLimit;	// max search depth of last search
public:
	void setDepthLimit(int depthLimit) { m_depthLimit = depthLimit; }
	void incDepthLimit() { m_depthLimit++; }
	int getDepthLimit() { return(m_depthLimit); }

};
