NTSTATUS
HF_IoEscribirCalibrado(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);
NTSTATUS
HF_IoDescalibrar(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	);

NTSTATUS
HF_IoRecalibrar(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID Calibrado(
	PITFDEVICE_EXTENSION idevExt,
	PHID_INPUT_DATA datosHID,
	BOOLEAN esX52);

VOID SensibilidadYMapeado(
	PITFDEVICE_EXTENSION devExt,
	BOOLEAN esX52,
	PHID_INPUT_DATA entrada,
	PHID_INPUT_DATA salida
	);

VOID AccionEje
(	PDEVICE_EXTENSION devExt,
	BOOLEAN esX52,
	UCHAR idx,
	USHORT nuevo);

#ifdef _EJES_

typedef	struct _CALIBRADO {
	UINT16 i;
	UINT16 c;
	UINT16 d;
	UCHAR n;
	UCHAR Margen;
	UCHAR Resistencia;
	BOOLEAN cal;
	BOOLEAN antiv;
} CALIBRADO, *PCALIBRADO;

UCHAR TraducirGiratorio(
	PITFDEVICE_EXTENSION idevExt,
	BOOLEAN esX52,
	UCHAR eje,
	USHORT nueva
	);

#endif