NTSTATUS
HF_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef _IOCTL_

typedef struct _READ_CONTEXT{
    
    KDPC			ReadTimerDpc;
    KTIMER			ReadTimer;
    PIRP			Irp;
	PDEVICE_OBJECT	DeviceObject;
  
} READ_CONTEXT, * PREAD_CONTEXT;

NTSTATUS 
ReadReport(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );

VOID 
DpcRoutineLectura(   
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#endif