#pragma once

class CTimer {
public:
	CTimer(void) {
		m_bDisabled = false;
	}
	//~CTimer(void);
private:
	unsigned long m_begTime;
	bool m_bDisabled;
public:
	void start();
	void setDisable(bool bDisable);
	bool getDisable() { return(m_bDisabled); }

	unsigned long elapsedTime();

	bool isTimeout(unsigned long seconds);
};


