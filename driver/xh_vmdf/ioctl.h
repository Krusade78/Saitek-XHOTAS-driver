NTSTATUS
HF_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef _IOCTL_

typedef struct _READ_WORKITEM
{
    PIO_WORKITEM      WorkItem;
    PDEVICE_EXTENSION DeviceExtension;
} READ_WORKITEM, *PREAD_WORKITEM;
typedef struct _READ_CONTEXT
{   
//    KDPC			ReadTimerDpc;
//    KTIMER			ReadTimer;
//    PIRP			Irp;
//	PDEVICE_OBJECT	DeviceObject;
	KEVENT				Ev;
	PETHREAD			Hilo;
	PDEVICE_EXTENSION	DeviceExtension;
  
} READ_CONTEXT, * PREAD_CONTEXT;

NTSTATUS 
ReadReport(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );

VOID
WIReadReport(
	IN PDEVICE_OBJECT DevObj,
	IN PVOID Context
	);

VOID 
DpcRoutineLectura(IN PVOID Context);

#endif