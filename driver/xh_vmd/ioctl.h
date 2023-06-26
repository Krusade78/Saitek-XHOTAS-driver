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
	// Keyboard descriptor
    0x05,   0x01,       // Usage Page (Generic Desktop),
    0x09,   0x06,       // Usage (Keyboard),
    0xA1,   0x01,       // Collection (Application),
    0x85, 0x01,   // Report ID 01.
    0x05,   0x07,       //  Usage Page (Key Codes);
    0x19,   0x00,       //  Usage Minimum (0),
    0x29,   0xe7,       //  Usage Maximum (231),
    0x15,   0x00,       //  Logical Minimum (0),
    0x25,   0x01,       //  Logical Maximum (1),
    0x75,   0x01,       //  Report Size (1),
    0x95,   0xe8,       //  Report Count (232),
    0x81,   0x02,       //  Input (Data, Variable, Absolute),;Modifier byte
    0xC0,                // End Collection

	// Mouse Descriptor
    0x05, 0x01, // Usage Page (Generic Desktop),
    0x09, 0x02, // Usage (Mouse),
    0xA1, 0x01, // Collection (Application),
	0x85, 0x02,   // Report ID 02.
    0x09, 0x01, // Usage (Pointer),
    0xA1, 0x00, // Collection (Physical),
    0x05, 0x09, // Usage Page (Buttons),
    0x19, 0x01, // Usage Minimum (01),
    0x29, 0x03, // Usage Maximun (03),
    0x15, 0x00, // Logical Minimum (0),
    0x25, 0x01, // Logical Maximum (1),
    0x95, 0x03, // Report Count (3),
    0x75, 0x01, // Report Size (1),
    0x81, 0x02, // Input (Data, Variable, Absolute), ;3 button bits
    0x95, 0x01, // Report Count (1),
    0x75, 0x05, // Report Size (5),
    0x81, 0x03, // Input (Constant), ;5 bit padding
    0x05, 0x01, // Usage Page (Generic Desktop),
    0x09, 0x30, // Usage (X),
    0x09, 0x31, // Usage (Y),
	0x09, 0x38,	//Wheel
    0x15, 0x81, // Logical Minimum (-127),
    0x25, 0x7f, // Logical Maximum (127),
    0x75, 0x08, // Report Size (8),
    0x95, 0x03, // Report Count (2),
    0x81, 0x06, // Input (Data, Variable, Relative), ;2 position bytes (X & Y)
    0xC0, // End Collection,
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