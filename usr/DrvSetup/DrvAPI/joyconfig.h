#pragma once

class CjoyConfig
{
public:
	CjoyConfig();

	bool Instalar(DWORD s_int, bool x35);
	void/*bool*/ Desinstalar();
private:
	bool GuardarRegistro(BYTE* ruta,BYTE* datos,DWORD tam);
	bool BorrarRegistro();
	bool LeerRegistro(char** interfaz,PVOID* puerto);
};