#pragma once

class CInstalar
{
public:
	CInstalar();
	~CInstalar(void);
	bool InstalarTeclado(DWORD s_teclado);
	bool InstalarWrapper(HWND hwnd);
	bool InstalarVHID(HWND hwnd);
	bool InstalarJoystickUSB();
	bool InstalarJoystick(HWND hWnd,HINSTANCE hInst, DWORD s_interfaz,bool x35);
private:
	bool Wrapper(HWND hwnd);
	bool VHID(HWND hwnd);
	bool FiltroTeclado(DWORD s_teclado);
	bool JoystickUSB();
	bool Joystick(DWORD s_interfaz, bool x35);
	void Error(char* tit);
	void Error(char* tit,HRESULT res);
};
