#pragma once

class CjoyConfig
{
public:
	CjoyConfig();
	virtual ~CjoyConfig();

	bool Comprobar();
private:
	void Error(char* tit);
	void Error(char* tit, HRESULT res);
	bool LeerRegistro(char** interfaz, BYTE** gameenum,UINT16 gametam);
	bool GuardarRegistro(BYTE* datos,DWORD tam);
};