//#include <iostream.h>
#include "Timer.h"
#include <time.h>


void CTimer::start() {
	m_begTime = clock();
}

void CTimer::disable(bool bDisable) {
	m_bDisabled = bDisable;
}

unsigned long CTimer::elapsedTime() {
	return ((unsigned long) clock() - m_begTime) / CLOCKS_PER_SEC;
}

bool CTimer::isTimeout(unsigned long seconds) {
	return ((seconds < elapsedTime()) && !m_bDisabled);
}


