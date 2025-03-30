/*
 * Human Interface Device class driver
 *
 * Copyright 2015 Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include "initguid.h"
#include "hid.h"
#include "devguid.h"
#include "ntddmou.h"
#include "ntddkbd.h"
#include "ddk/hidtypes.h"
#include "ddk/wdm.h"
#include "regstr.h"
#include "ntuser.h"
#include "wine/debug.h"
#include "wine/asm.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

DEFINE_GUID(GUID_DEVINTERFACE_WINEXINPUT, 0x6c53d5fd, 0x6480, 0x440f, 0xb6, 0x18, 0x47, 0x67, 0x50, 0xc5, 0xe1, 0xa6);

#ifdef __ASM_USE_FASTCALL_WRAPPER

extern void * WINAPI wrap_fastcall_func1( void *func, const void *a );
__ASM_STDCALL_FUNC( wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax" );

#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)

#else

#define call_fastcall_func1(func,a) func(a)

#endif

static struct list minidriver_list = LIST_INIT(minidriver_list);

static minidriver *find_minidriver(DRIVER_OBJECT *driver)
{
    minidriver *md;
    LIST_FOR_EACH_ENTRY(md, &minidriver_list, minidriver, entry)
    {
        if (md->minidriver.DriverObject == driver)
            return md;
    }
    return NULL;
}

static NTSTATUS get_device_id(DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR *id)
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    KEVENT event;
    IRP *irp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, device, NULL, 0, NULL, &event, &irp_status);
    if (irp == NULL)
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->MinorFunction = IRP_MN_QUERY_ID;
    irpsp->Parameters.QueryId.IdType = type;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!irp_status.Status)
    {
        wcscpy(id, (WCHAR *)irp_status.Information);
        ExFreePool((WCHAR *)irp_status.Information);
    }

    return irp_status.Status;
}

/* user32 reserves 1 & 2 for winemouse and winekeyboard,
 * keep this in sync with user_private.h */
#define WINE_MOUSE_HANDLE 1
#define WINE_KEYBOARD_HANDLE 2

static UINT32 alloc_rawinput_handle(void)
{
    static LONG counter = WINE_KEYBOARD_HANDLE + 1;
    return InterlockedIncrement(&counter);
}

/* make sure bRawData can hold UsagePage and Usage without requiring additional allocation */
C_ASSERT(offsetof(RAWINPUT, data.hid.bRawData[2 * sizeof(USAGE)]) < sizeof(RAWINPUT));

static void send_wm_input_device_change(BASE_DEVICE_EXTENSION *ext, LPARAM param)
{
    HIDP_COLLECTION_DESC *desc = ext->u.pdo.collection_desc;
    RAWINPUT rawinput;
    INPUT input;

    TRACE("ext %p, lparam %p\n", ext, (void *)param);

    if (!IsEqualGUID( ext->class_guid, &GUID_DEVINTERFACE_HID )) return;

    rawinput.header.dwType = RIM_TYPEHID;
    rawinput.header.dwSize = offsetof(RAWINPUT, data.hid.bRawData[2 * sizeof(USAGE)]);
    rawinput.header.hDevice = ULongToHandle(ext->u.pdo.rawinput_handle);
    rawinput.header.wParam = param;
    rawinput.data.hid.dwCount = 0;
    rawinput.data.hid.dwSizeHid = 0;
    ((USAGE *)rawinput.data.hid.bRawData)[0] = desc->UsagePage;
    ((USAGE *)rawinput.data.hid.bRawData)[1] = desc->Usage;

    input.type = INPUT_HARDWARE;
    input.hi.uMsg = WM_INPUT_DEVICE_CHANGE;
    input.hi.wParamH = 0;
    input.hi.wParamL = 0;
    __wine_send_input(0, &input, &rawinput);
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *bus_pdo)
{
    WCHAR device_id[MAX_DEVICE_ID_LEN], instance_id[MAX_DEVICE_ID_LEN];
    BASE_DEVICE_EXTENSION *ext;
    BOOL is_xinput_class;
    DEVICE_OBJECT *fdo;
    NTSTATUS status;
    minidriver *minidriver;

    if ((status = get_device_id(bus_pdo, BusQueryDeviceID, device_id)))
    {
        ERR( "Failed to get PDO device id, status %#lx.\n", status );
        return status;
    }

    if ((status = get_device_id(bus_pdo, BusQueryInstanceID, instance_id)))
    {
        ERR( "Failed to get PDO instance id, status %#lx.\n", status );
        return status;
    }

    TRACE("Adding device to PDO %p, id %s\\%s.\n", bus_pdo, debugstr_w(device_id), debugstr_w(instance_id));
    minidriver = find_minidriver(driver);

    if ((status = IoCreateDevice(driver, sizeof(*ext) + minidriver->minidriver.DeviceExtensionSize,
            NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &fdo)))
    {
        ERR( "Failed to create bus FDO, status %#lx.\n", status );
        return status;
    }
    ext = fdo->DeviceExtension;
    ext->is_fdo = TRUE;
    ext->u.fdo.hid_ext.MiniDeviceExtension = ext + 1;
    ext->u.fdo.hid_ext.PhysicalDeviceObject = bus_pdo;
    ext->u.fdo.hid_ext.NextDeviceObject = bus_pdo;
    swprintf(ext->device_id, ARRAY_SIZE(ext->device_id), L"HID\\%s", wcsrchr(device_id, '\\') + 1);
    wcscpy(ext->instance_id, instance_id);

    if (get_device_id(bus_pdo, BusQueryContainerID, ext->container_id))
        ext->container_id[0] = 0;

    ext->steam_overlay_event = minidriver->steam_overlay_event;
    ext->steam_keyboard_event = minidriver->steam_keyboard_event;

    is_xinput_class = !wcsncmp(device_id, L"WINEXINPUT\\", 7) && wcsstr(device_id, L"&XI_") != NULL;
    if (is_xinput_class) ext->class_guid = &GUID_DEVINTERFACE_WINEXINPUT;
    else ext->class_guid = &GUID_DEVINTERFACE_HID;

    status = minidriver->AddDevice(minidriver->minidriver.DriverObject, fdo);
    if (status != STATUS_SUCCESS)
    {
        ERR( "Minidriver AddDevice failed (%lx)\n", status );
        IoDeleteDevice(fdo);
        return status;
    }

    IoAttachDeviceToDeviceStack(fdo, bus_pdo);
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static NTSTATUS get_hid_device_desc( minidriver *minidriver, DEVICE_OBJECT *device, HIDP_DEVICE_DESC *desc )
{
    HID_DESCRIPTOR descriptor = {0};
    IO_STATUS_BLOCK io;
    BYTE *report_desc;
    INT i;

    call_minidriver( IOCTL_HID_GET_DEVICE_DESCRIPTOR, device, NULL, 0, &descriptor, sizeof(descriptor), &io );
    if (io.Status) return io.Status;

    for (i = 0; i < descriptor.bNumDescriptors; i++)
        if (descriptor.DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE)
            break;
    if (i >= descriptor.bNumDescriptors) return STATUS_NOT_FOUND;

    if (!(report_desc = malloc( descriptor.DescriptorList[i].wReportLength ))) return STATUS_NO_MEMORY;
    call_minidriver( IOCTL_HID_GET_REPORT_DESCRIPTOR, device, NULL, 0, report_desc,
                     descriptor.DescriptorList[i].wReportLength, &io );
    if (!io.Status) io.Status = HidP_GetCollectionDescription( report_desc, descriptor.DescriptorList[i].wReportLength,
                                                               PagedPool, desc );
    free( report_desc );

    if (io.Status && io.Status != HIDP_STATUS_SUCCESS) return STATUS_INVALID_PARAMETER;
    return STATUS_SUCCESS;
}

static NTSTATUS initialize_device( minidriver *minidriver, DEVICE_OBJECT *device )
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    ULONG index = HID_STRING_ID_ISERIALNUMBER;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    call_minidriver( IOCTL_HID_GET_DEVICE_ATTRIBUTES, device, NULL, 0, &ext->u.fdo.attrs,
                     sizeof(ext->u.fdo.attrs), &io );
    if (io.Status != STATUS_SUCCESS)
    {
        ERR( "Minidriver failed to get attributes, status %#lx.\n", io.Status );
        return io.Status;
    }

    call_minidriver( IOCTL_HID_GET_STRING, device, ULongToPtr(index), sizeof(index),
                     &ext->u.fdo.serial, sizeof(ext->u.fdo.serial), &io );
    if (io.Status != STATUS_SUCCESS)
    {
        ERR( "Minidriver failed to get serial number, status %#lx.\n", io.Status );
        return io.Status;
    }

    if ((status = get_hid_device_desc( minidriver, device, &ext->u.fdo.device_desc )))
    {
        ERR( "Failed to get HID device description, status %#lx\n", status );
        return status;
    }

    if (!(ext->u.fdo.child_pdos = malloc( ext->u.fdo.device_desc.CollectionDescLength * sizeof(ext->u.fdo.child_pdos) )))
    {
        ERR( "Cannot allocate child PDOs array\n" );
        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS create_child_pdos( minidriver *minidriver, DEVICE_OBJECT *device )
{
    BASE_DEVICE_EXTENSION *fdo_ext = device->DeviceExtension, *pdo_ext;
    DEVICE_OBJECT *child_pdo;
    UNICODE_STRING string;
    WCHAR pdo_name[255];
    USAGE page, usage;
    NTSTATUS status;
    INT i;

    for (i = 0; i < fdo_ext->u.fdo.device_desc.CollectionDescLength; ++i)
    {
        if (fdo_ext->u.fdo.device_desc.CollectionDescLength > 1)
            swprintf( pdo_name, ARRAY_SIZE(pdo_name), L"\\Device\\HID#%p&%p&%d", device->DriverObject,
                      fdo_ext->u.fdo.hid_ext.PhysicalDeviceObject, i );
        else
            swprintf( pdo_name, ARRAY_SIZE(pdo_name), L"\\Device\\HID#%p&%p", device->DriverObject,
                      fdo_ext->u.fdo.hid_ext.PhysicalDeviceObject );

        RtlInitUnicodeString(&string, pdo_name);
        if ((status = IoCreateDevice( device->DriverObject, sizeof(*pdo_ext), &string, 0, 0, FALSE, &child_pdo )))
        {
            ERR( "Failed to create child PDO, status %#lx.\n", status );
            return status;
        }

        fdo_ext->u.fdo.child_pdos[i] = child_pdo;
        fdo_ext->u.fdo.child_count++;

        pdo_ext = child_pdo->DeviceExtension;
        pdo_ext->u.pdo.parent_fdo = device;
        list_init( &pdo_ext->u.pdo.queues );
        KeInitializeSpinLock( &pdo_ext->u.pdo.queues_lock );

        pdo_ext->u.pdo.collection_desc = fdo_ext->u.fdo.device_desc.CollectionDesc + i;

        if (fdo_ext->u.fdo.device_desc.CollectionDescLength > 1)
        {
            swprintf( pdo_ext->device_id, ARRAY_SIZE(pdo_ext->device_id), L"%s&Col%02d",
                      fdo_ext->device_id, pdo_ext->u.pdo.collection_desc->CollectionNumber );
            swprintf( pdo_ext->instance_id, ARRAY_SIZE(pdo_ext->instance_id), L"%u&%s&%x&%u&%04u",
                      fdo_ext->u.fdo.attrs.VersionNumber, fdo_ext->u.fdo.serial, 0, 0, i );
        }
        else
        {
            wcscpy( pdo_ext->device_id, fdo_ext->device_id );
            wcscpy( pdo_ext->instance_id, fdo_ext->instance_id );
        }
        wcscpy(pdo_ext->container_id, fdo_ext->container_id);
        pdo_ext->class_guid = fdo_ext->class_guid;

        pdo_ext->steam_overlay_event = minidriver->steam_overlay_event;
        pdo_ext->steam_keyboard_event = minidriver->steam_keyboard_event;

        pdo_ext->u.pdo.information.VendorID = fdo_ext->u.fdo.attrs.VendorID;
        pdo_ext->u.pdo.information.ProductID = fdo_ext->u.fdo.attrs.ProductID;
        pdo_ext->u.pdo.information.VersionNumber = fdo_ext->u.fdo.attrs.VersionNumber;
        pdo_ext->u.pdo.information.Polled = minidriver->minidriver.DevicesArePolled;
        pdo_ext->u.pdo.information.DescriptorSize = pdo_ext->u.pdo.collection_desc->PreparsedDataLength;

        page = pdo_ext->u.pdo.collection_desc->UsagePage;
        usage = pdo_ext->u.pdo.collection_desc->Usage;
        if (page == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_MOUSE)
            pdo_ext->u.pdo.rawinput_handle = WINE_MOUSE_HANDLE;
        else if (page == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_KEYBOARD)
            pdo_ext->u.pdo.rawinput_handle = WINE_KEYBOARD_HANDLE;
        else
            pdo_ext->u.pdo.rawinput_handle = alloc_rawinput_handle();
        pdo_ext->u.pdo.poll_interval = DEFAULT_POLL_INTERVAL;

        TRACE( "created device %p, rawinput handle %#x\n", pdo_ext, pdo_ext->u.pdo.rawinput_handle );
    }

    IoInvalidateDeviceRelations( fdo_ext->u.fdo.hid_ext.PhysicalDeviceObject, BusRelations );
    return STATUS_SUCCESS;
}

static NTSTATUS fdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    minidriver *minidriver = find_minidriver(device->DriverObject);
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    NTSTATUS status;

    TRACE("irp %p, minor function %#x.\n", irp, stack->MinorFunction);

    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DEVICE_RELATIONS *devices;
            UINT32 i;

            if (stack->Parameters.QueryDeviceRelations.Type != BusRelations)
                return minidriver->PNPDispatch(device, irp);

            if (!(devices = ExAllocatePool(PagedPool, offsetof(DEVICE_RELATIONS, Objects[ext->u.fdo.child_count]))))
            {
                irp->IoStatus.Status = STATUS_NO_MEMORY;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return STATUS_NO_MEMORY;
            }

            for (i = 0, devices->Count = 0; i < ext->u.fdo.child_count; ++i)
            {
                devices->Objects[i] = ext->u.fdo.child_pdos[i];
                call_fastcall_func1(ObfReferenceObject, ext->u.fdo.child_pdos[i]);
                devices->Count++;
            }

            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            return IoCallDriver(ext->u.fdo.hid_ext.NextDeviceObject, irp);
        }

        case IRP_MN_START_DEVICE:
            status = minidriver->PNPDispatch( device, irp );
            if (!status) status = initialize_device( minidriver, device );
            if (!status) status = create_child_pdos( minidriver, device );
            return status;

        case IRP_MN_REMOVE_DEVICE:
            status = minidriver->PNPDispatch( device, irp );
            HidP_FreeCollectionDescription( &ext->u.fdo.device_desc );
            free( ext->u.fdo.child_pdos );
            IoDetachDevice( ext->u.fdo.hid_ext.NextDeviceObject );
            IoDeleteDevice( device );
            return status;

        default:
            return minidriver->PNPDispatch(device, irp);
    }
}

static NTSTATUS pdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    HIDP_COLLECTION_DESC *desc = ext->u.pdo.collection_desc;
    NTSTATUS status = irp->IoStatus.Status;
    struct hid_queue *queue, *next;
    KIRQL irql;

    TRACE("irp %p, minor function %#x.\n", irp, irpsp->MinorFunction);

    switch(irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
        {
            WCHAR *id = ExAllocatePool(PagedPool, sizeof(WCHAR) * REGSTR_VAL_MAX_HCID_LEN);
            TRACE("IRP_MN_QUERY_ID[%i]\n", irpsp->Parameters.QueryId.IdType);
            switch (irpsp->Parameters.QueryId.IdType)
            {
                case BusQueryHardwareIDs:
                case BusQueryCompatibleIDs:
                {
                    WCHAR *ptr;
                    ptr = id;
                    /* Device instance ID */
                    lstrcpyW(ptr, ext->device_id);
                    ptr += lstrlenW(ext->device_id);
                    lstrcpyW(ptr, L"\\");
                    ptr += 1;
                    lstrcpyW(ptr, ext->instance_id);
                    ptr += lstrlenW(ext->instance_id) + 1;
                    /* Device ID */
                    lstrcpyW(ptr, ext->device_id);
                    ptr += lstrlenW(ext->device_id) + 1;
                    /* Bus ID */
                    lstrcpyW(ptr, L"HID");
                    ptr += lstrlenW(L"HID") + 1;
                    *ptr = 0;
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    status = STATUS_SUCCESS;
                    break;
                }
                case BusQueryDeviceID:
                    lstrcpyW(id, ext->device_id);
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    status = STATUS_SUCCESS;
                    break;
                case BusQueryInstanceID:
                    lstrcpyW(id, ext->instance_id);
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    status = STATUS_SUCCESS;
                    break;
                case BusQueryContainerID:
                    if (ext->container_id[0])
                    {
                        lstrcpyW(id, ext->container_id);
                        irp->IoStatus.Information = (ULONG_PTR)id;
                        status = STATUS_SUCCESS;
                    }
                    else
                    {
                        ExFreePool(id);
                    }
                    break;

                case BusQueryDeviceSerialNumber:
                    FIXME("unimplemented id type %#x\n", irpsp->Parameters.QueryId.IdType);
                    ExFreePool(id);
                    break;
            }
            break;
        }

        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = irpsp->Parameters.DeviceCapabilities.Capabilities;

            caps->RawDeviceOK = 1;
            status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_START_DEVICE:
            HID_StartDeviceThread(device);
            send_wm_input_device_change(ext, GIDC_ARRIVAL);

            if ((status = IoRegisterDeviceInterface(device, ext->class_guid, NULL, &ext->u.pdo.link_name)))
            {
                ERR( "Failed to register interface, status %#lx.\n", status );
                break;
            }

            /* FIXME: This should probably be done in mouhid.sys. */
            if (desc->UsagePage == HID_USAGE_PAGE_GENERIC && desc->Usage == HID_USAGE_GENERIC_MOUSE)
            {
                if (!IoRegisterDeviceInterface(device, &GUID_DEVINTERFACE_MOUSE, NULL, &ext->u.pdo.mouse_link_name))
                    ext->u.pdo.is_mouse = TRUE;
            }
            if (desc->UsagePage == HID_USAGE_PAGE_GENERIC && desc->Usage == HID_USAGE_GENERIC_KEYBOARD)
            {
                if (!IoRegisterDeviceInterface(device, &GUID_DEVINTERFACE_KEYBOARD, NULL, &ext->u.pdo.keyboard_link_name))
                    ext->u.pdo.is_keyboard = TRUE;
            }

            IoSetDeviceInterfaceState(&ext->u.pdo.link_name, TRUE);
            if (ext->u.pdo.is_mouse)
                IoSetDeviceInterfaceState(&ext->u.pdo.mouse_link_name, TRUE);
            if (ext->u.pdo.is_keyboard)
                IoSetDeviceInterfaceState(&ext->u.pdo.keyboard_link_name, TRUE);

            ext->u.pdo.removed = FALSE;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            send_wm_input_device_change(ext, GIDC_REMOVAL);

            IoSetDeviceInterfaceState(&ext->u.pdo.link_name, FALSE);
            if (ext->u.pdo.is_mouse)
                IoSetDeviceInterfaceState(&ext->u.pdo.mouse_link_name, FALSE);
            if (ext->u.pdo.is_keyboard)
                IoSetDeviceInterfaceState(&ext->u.pdo.keyboard_link_name, FALSE);

            if (ext->u.pdo.thread)
            {
                SetEvent(ext->u.pdo.halt_event);
                WaitForSingleObject(ext->u.pdo.thread, INFINITE);
            }
            CloseHandle(ext->u.pdo.halt_event);

            KeAcquireSpinLock( &ext->u.pdo.queues_lock, &irql );
            LIST_FOR_EACH_ENTRY_SAFE( queue, next, &ext->u.pdo.queues, struct hid_queue, entry )
                hid_queue_destroy( queue );
            KeReleaseSpinLock( &ext->u.pdo.queues_lock, irql );

            RtlFreeUnicodeString(&ext->u.pdo.link_name);

            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            IoDeleteDevice(device);
            return STATUS_SUCCESS;

        case IRP_MN_SURPRISE_REMOVAL:
            KeAcquireSpinLock(&ext->u.pdo.lock, &irql);
            ext->u.pdo.removed = TRUE;
            KeReleaseSpinLock(&ext->u.pdo.lock, irql);

            KeAcquireSpinLock( &ext->u.pdo.queues_lock, &irql );
            LIST_FOR_EACH_ENTRY_SAFE( queue, next, &ext->u.pdo.queues, struct hid_queue, entry )
                hid_queue_remove_pending_irps( queue );
            KeReleaseSpinLock( &ext->u.pdo.queues_lock, irql );

            SetEvent(ext->u.pdo.halt_event);
            status = STATUS_SUCCESS;
            break;

        default:
            FIXME("Unhandled minor function %#x.\n", irpsp->MinorFunction);
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    if (ext->is_fdo)
        return fdo_pnp(device, irp);
    else
        return pdo_pnp(device, irp);
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    if (ext->is_fdo)
    {
        irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }

    return pdo_create(device, irp);
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_close(device, irp);
}

static NTSTATUS WINAPI driver_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_ioctl(device, irp);
}

static NTSTATUS WINAPI driver_read(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_read(device, irp);
}

static NTSTATUS WINAPI driver_write(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_write(device, irp);
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    minidriver *md;

    TRACE("\n");

    if ((md = find_minidriver(driver)))
    {
        if (md->DriverUnload)
            md->DriverUnload(md->minidriver.DriverObject);
        list_remove(&md->entry);

        CloseHandle(md->steam_overlay_event);
        CloseHandle(md->steam_keyboard_event);
        free(md);
    }
}

NTSTATUS WINAPI HidRegisterMinidriver(HID_MINIDRIVER_REGISTRATION *registration)
{
    minidriver *driver;

    /* make sure we have a window station and a desktop, we need one to send input */
    if (!GetProcessWindowStation())
        return STATUS_INVALID_PARAMETER;

    if (!(driver = calloc(1, sizeof(*driver))))
        return STATUS_NO_MEMORY;

    driver->steam_overlay_event = CreateEventA(NULL, TRUE, FALSE, "__wine_steamclient_GameOverlayActivated");
    driver->steam_keyboard_event = CreateEventA(NULL, TRUE, FALSE, "__wine_steamclient_KeyboardActivated");

    driver->DriverUnload = registration->DriverObject->DriverUnload;
    registration->DriverObject->DriverUnload = driver_unload;

    registration->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    registration->DriverObject->MajorFunction[IRP_MJ_READ] = driver_read;
    registration->DriverObject->MajorFunction[IRP_MJ_WRITE] = driver_write;
    registration->DriverObject->MajorFunction[IRP_MJ_CREATE] = driver_create;
    registration->DriverObject->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    driver->PNPDispatch = registration->DriverObject->MajorFunction[IRP_MJ_PNP];
    registration->DriverObject->MajorFunction[IRP_MJ_PNP] = driver_pnp;

    driver->AddDevice = registration->DriverObject->DriverExtension->AddDevice;
    registration->DriverObject->DriverExtension->AddDevice = driver_add_device;

    driver->minidriver = *registration;
    list_add_tail(&minidriver_list, &driver->entry);

    return STATUS_SUCCESS;
}

void call_minidriver( ULONG code, DEVICE_OBJECT *device, void *in_buff, ULONG in_size,
                      void *out_buff, ULONG out_size, IO_STATUS_BLOCK *io )
{
    IRP *irp;
    KEVENT event;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest( code, device, in_buff, in_size, out_buff, out_size, TRUE, &event, io );

    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
}
