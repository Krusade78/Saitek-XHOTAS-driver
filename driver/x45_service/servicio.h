/*--
Copyright (c) 2007 Alfredo Costalago

Module Name:

    serviciox36.h
--*/

typedef	struct _CALIBRADO {
	UINT16 i;
	UINT16 c;
	UINT16 d;
	UCHAR n;
	UCHAR Margen;
	UCHAR Resistencia;
	BOOLEAN cal;
	BOOLEAN antiv;
} CALIBRADO;

class CServicio
{
public:
	CServicio();

	void IniciarServicio();
private:
	void CargarCalibrado();
	void LeerRegistro(CALIBRADO* ejes);
};