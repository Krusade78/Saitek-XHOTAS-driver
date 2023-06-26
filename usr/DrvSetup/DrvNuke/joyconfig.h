#pragma once

class CjoyConfig
{
public:
	CjoyConfig();

	void/*bool*/ Desinstalar();
private:
	bool BorrarRegistro();
	bool LeerRegistro(char** interfaz,PVOID* puerto);
};