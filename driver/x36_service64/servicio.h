/*--
Copyright (c) 2007 Alfredo Costalago

Module Name:

    serviciox36.h
--*/

typedef	struct _CALIBRADOX36 {
	ULONG i;
	ULONG c;
	ULONG d;
	UCHAR n;
	UCHAR Margen;
	UCHAR Resistencia;
	BOOLEAN cal;
	BOOLEAN antiv;
} CALIBRADOX36;
typedef	struct _CALIBRADOHID {
	UINT16 i;
	UINT16 c;
	UINT16 d;
	UCHAR n;
	UCHAR Margen;
	UCHAR Resistencia;
	BOOLEAN cal;
	BOOLEAN antiv;
} CALIBRADOHID;

class CServicio
{
public:
	CServicio();

	BOOL IniciarServicio();
	void IniciarX36();
private:
	bool IniciarJoysticks();
	void CargarCalibradoX36();
	void LeerRegistroX36(CALIBRADOX36* ejes);
	void CargarCalibradoHID();
	void LeerRegistroHID(CALIBRADOHID* ejes);
};