#pragma once

class CDesinstalar
{
public:
	CDesinstalar();
	~CDesinstalar(void);
	void Iniciar();
private:
	bool Wrapper();
	bool VHID();
	bool Joystick();
	bool JoystickUSB();
	bool FiltroTeclado();
	bool Base();
//private:
	//HWND hWnd;
	//HINSTANCE hInst;
	//void Error(char* tit);
};
