NTSTATUS
HF_ControlDispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

//Funciones privadas
#ifdef _CONTROL_

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

NTSTATUS
HF_IoLeerEstado(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HF_IoLeerCalibrado(
	IN PDEVICE_OBJECT DEviceObject,
	IN PIRP Irp
	);

NTSTATUS
HF_IoEscribirCalibrado(
	IN PDEVICE_OBJECT DEviceObject,
	IN PIRP Irp
	);

#endif