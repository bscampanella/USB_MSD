Hiperwall USB Viewer

Developer: Brandon Campanella
Original SDK: Microsoft
Date: 9/13/2018

Original SDK "USBView" published by Microsoft at https://github.com/Microsoft/Windows-driver-samples/tree/master/usb/usbview
The license can be found at https://github.com/Microsoft/Windows-driver-samples/blob/master/LICENSE.
Notablely  "Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3,
each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use,
sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the
contribution in the software."

Description from original README
Code tour
---------

File manifest | Description 
--------------|------------
Resource.h | ID definitions for GUI controls 
Usbdesc.h | USB descriptor type definitions 
Usbview.h | Main header file for this sample 
Vndrlist.h | List of USB Vendor IDs and vendor names 
Debug.c | Assertion routines for the checked build 
Devnode.c | Routines for accessing DevNode information 
Dispaud.c | Routines for displaying USB audio class device information 
Enum.c | Routines for displaying USB device information 
Uvcview.c | Entry point and GUI handling routines 

The major topics covered in this tour are:

-   GUI handling routines
-   Device enumeration routines
-   Device information display routines

The file Usbview.c contains the sample application entry point and GUI handling routines. On entry, the main application
window is created, which is actually a dialog box as defined in Usbview.rc. The dialog box consists of a split window with
a tree view control on the left side and an edit control on the right side.

The routine RefreshTree() is called to enumerate USB host controller, hubs, and attached devices and to populate the device
tree view control. RefreshTree() calls the routine EnumerateHostControllers() 
in Enum.c to enumerate USB host controller, hubs, and attached devices. After the device tree view control has been populated,
USBView\_OnNotify() is called when an item is selected in the device tree view control. This calls UpdateEditControl() in Display.c 
to display information about the selected item in the edit control.

The file Enum.c contains the routines that enumerate the USB bus and populate the tree view control. The USB device enumeration
and information collection process is the main point of this sample application. The enumeration process starts at EnumerateHostControllers() and goes like this:

1.  Enumerate Host Controllers and Root Hubs. Host controllers have symbolic link names of the form HCDx, where x starts at 0. Use CreateFile() to open each host controller symbolic link. Create a node in the tree view to represent each host controller. After a host controller has been opened, send the host controller an IOCTL\_USB\_GET\_ROOT\_HUB\_NAME request to get the symbolic link name of the root hub that is part of the host controller.
2.  Enumerate Hubs (Root Hubs and External Hubs). Given the name of a hub, use CreateFile() to open the hub. Send the hub an IOCTL\_USB\_GET\_NODE\_INFORMATION request to get info about the hub, such as the number of downstream ports. Create a node in the tree view to represent each hub.
3.  Enumerate Downstream Ports. Given a handle to an open hub and the number of downstream ports on the hub, send the hub an IOCTL\_USB\_GET\_NODE\_CONNECTION\_INFORMATION request for each downstream port of the hub to get info about the device (if any) attached to each port. If there is a device attached to a port, send the hub an IOCTL\_USB\_GET\_NODE\_CONNECTION\_NAME request to get the symbolic link name of the hub attached to the downstream port. If there is a hub attached to the downstream port, recurse to step (2). Create a node in the tree view to represent each hub port and attached device. USB configuration and string descriptors are retrieved from attached devices in GetConfigDescriptor() and GetStringDescriptor() by sending an IOCTL\_USB\_GET\_DESCRIPTOR\_FROM\_NODE\_CONNECTION() to the hub to which the device is attached.

The file Display.c contains routines that display information about selected devices in the application edit control. Information about the device was collected during the enumeration of the device tree. This information includes USB device, configuration, and string descriptors and connection and configuration information that is maintained by the USB stack. The routines in this file simply parse and print the data structures for the device that were collected when it was enumerated. The file Dispaud.c parses and prints data structures that are specific to USB audio class devices.

//end original read me


The majority of modifications to the original code was made in uvcview.c, with other minor modifications being made
in display.c and uvcview.rc . The Doxigen covers uvcview.rc extenxively, and the other files not so much as it is 
both intutive in reguards to what those functions do, and does not really need to be modified.

As per Hiperwalls needs this application lists all usb drives in a list in the left window, and when selected displays
information in the right screen. Along the way this program decides if each usb is suitable to act as a Hiperwall key by
checking to see if the usb is writable and contains a serial number. This is all controled within uvcview.c . This program also
has the ability to save the usb information to the local computer and to the specific drive, one at a time by using
save selected, or all at onece by using save all. The lower left hand window then reflects this save by printing out 
a confirmation/failure of the save opperations and the specific paths that all usbs were saved to.