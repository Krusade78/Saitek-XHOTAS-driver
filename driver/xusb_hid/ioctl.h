
NTSTATUS
HF_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    BOOLEAN* unlock);

#ifdef _IOCTL_

NTSTATUS ConfigDescriptorCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context );
NTSTATUS ReportDescriptorCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context );
NTSTATUS ReportCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context );

typedef struct _READ_WORKITEM
{
    PIO_WORKITEM      WorkItem;
    PDEVICE_EXTENSION DeviceExtension;
    PIRP              Irp;
} READ_WORKITEM, *PREAD_WORKITEM;
typedef struct _READ_CONTEXT
{
    PDEVICE_EXTENSION DeviceExtension;
    PIRP             Irp;
	PETHREAD		 Hilo;
	KEVENT			 Ev;
} READ_CONTEXT, *PREAD_CONTEXT;
//typedef struct _ESTADO_WORKITEM
//{
//    PIO_WORKITEM      WorkItem;
//    PDEVICE_EXTENSION DeviceExtension;
//    USBD_PIPE_HANDLE  hPipe;
//} ESTADO_WORKITEM, *PESTADO_WORKITEM;
//typedef struct _ESTADO_CONTEXT
//{
//    PDEVICE_EXTENSION DeviceExtension;
//    USBD_PIPE_HANDLE  hPipe;
//	PETHREAD		 Hilo;
//	KEVENT			 Ev;
//} ESTADO_CONTEXT, *PESTADO_CONTEXT;
VOID PreReadReport(IN PDEVICE_OBJECT DevObj,IN PVOID Context);
VOID ReadReport(IN PVOID Context);
//VOID LecturaEstado(IN PDEVICE_EXTENSION devExt, IN USBD_PIPE_HANDLE hPipe, BOOLEAN* unlock);
//VOID PreLeerEstado(IN PDEVICE_OBJECT DevObj,IN PVOID Context);
//VOID LeerEstado(IN PVOID Context);
VOID Procesar(IN PDEVICE_EXTENSION devExt, IN PUCHAR buffer, IN ULONG tam);
//VOID BreakIrp(IN PDEVICE_EXTENSION devExt);

UCHAR reportDescriptor[] = {
		0x05, 0x01,
		0x09, 0x04,
		0xa1, 0x01,
		0x15, 0x00,
		0x26, 0xff, 0x0f,
		0x95, 0x01,
		0x75, 0x10,
		0x09, 0x30,  //X
		0x81, 0x02,
		0x09, 0x31,  //Y
		0x81, 0x02,
		0x09, 0x35,	//Rudder
		0x81, 0x02,
		0x09, 0x32,  //throttle
		0x81, 0x02,
		//0x15, 0x00,
		//0x26, 0xff,0x0f,
		//0x75, 0x10,
		0x09, 0x33,  //Rx
		0x81, 0x02,
		0x09, 0x34,  //Ry
		0x81, 0x02,
		0x09, 0x36,  //Slider
		0x81, 0x02,

		0x15, 0x01,
		0x25, 0x08,
		0x75, 0x08,
		0x35, 0x00,
		0x46, 0x3b, 0x01,
		0x65, 0x14,
		0x09, 0x39, //Hat1
		0x81, 0x42,
		0x09, 0x39, //Hat2
		0x81, 0x42,
		0x09, 0x39, //Hat3
		0x81, 0x42,
		0x09, 0x39, //Hat4
		0x81, 0x42,

		0x05,0x09,			//botones
		0x19,0x01,
		0x29,0x38,
		0x15,0x00,
		0x25,0x01,
		0x75,0x01,
		0x95,0x38,
		0x81,0x02,

		0x05,0x01, //ministick
			0x15,0x00,
			0x25,0x0f,
			0x35,0x00,
			0x45,0x0f,
			0x95,0x01,
			0x75,0x04,
			0x66,0x11,0xf0, //unit velicity
			0x09,0x30, //vx
			0x81,0x02,
			0x09,0x31, //vy
			0x81,0x02,

		0xc0
	};

#endif