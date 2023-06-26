NTSTATUS
HF_IoEscribirComandos
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HF_IoEscribirMapa(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID LimpiarMemoria(IN PVOID contexto);