//DRIVER_DISPATCH InternalIoctl;
NTSTATUS 
InternalIoctl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef _IOCTL_

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

//
// This is the default report descriptor for the virtual Hid device returned
// by the mini driver in response to IOCTL_HID_GET_REPORT_DESCRIPTOR if the
// driver fails to read the report descriptor from registry. 
//
HID_REPORT_DESCRIPTOR DefaultReportDescriptor[] = {
    0x06,   0x00, 0xff,  // Usage Page (Generic),
    0x09,   0x00,       // Usage (indefinido),
    0xA1,   0x01,       // Collection (Application),
    0xC0 // End Collection
};

NTSTATUS 
GetHidDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS 
GetReportDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS 
GetDeviceAttributes(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp);

#endif