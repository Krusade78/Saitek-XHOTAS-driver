NTSTATUS
HF_ControlDispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );

#ifdef _CONTROL_

NTSTATUS SetFecha( IN PDEVICE_OBJECT DeviceObject, IN UCHAR* datos);
NTSTATUS SetLinea( IN PDEVICE_OBJECT DeviceObject, IN CHAR linea, IN UCHAR* texto);
NTSTATUS EnviarOrden( IN PDEVICE_OBJECT DeviceObject, IN UCHAR* params);

#endif
