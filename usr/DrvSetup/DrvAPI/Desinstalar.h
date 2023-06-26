#pragma once

class CDesinstalar
{
public:
	CDesinstalar(HWND hWnd,HINSTANCE hInst);
	~CDesinstalar(void);
	bool Iniciar();
	bool Wrapper();
	bool VHID();
	bool Joystick();
	bool JoystickUSB();
	bool FiltroTeclado();
	bool Base();
private:
	HWND hWnd;
	HINSTANCE hInst;
	void Error(char* tit);
	bool esX36G();
};
