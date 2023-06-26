#pragma once

class CInstalar
{
public:
	CInstalar();
	~CInstalar(void);
	char PrepararDlgTeclados(HWND lista);
	char PrepararDlgInterfaces(HWND lista);
private:
	void Error(char* tit);
	void Error(char* tit,HRESULT res);
};
