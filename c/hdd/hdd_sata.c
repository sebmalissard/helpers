/* List USB storage devices using libudev.
 *
 * gcc -o udev_list_usb_storage udev_list_usb_storage.c -ludev
 * ./udev_list_usb_storage
 */
#include <libudev.h>
#include <stdio.h>
#include <string.h>


//https://stackoverflow.com/questions/27067595/get-dev-disk-by-id-element-from-dev-sd-element
static void enumerate_sata_disk()
{
    struct udev             *udev;
    struct udev_enumerate   *enumerate;
    struct udev_list_entry  *udev_entries;
    struct udev_list_entry  *dev_list_entry;
    struct udev_device      *dev;
    struct udev_device      *parent;
    
    udev = udev_new();
    enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_add_match_property(enumerate, "ID_TYPE", "disk");
    udev_enumerate_scan_devices(enumerate);
    udev_entries = udev_enumerate_get_list_entry(enumerate);    

    udev_list_entry_foreach(dev_list_entry, udev_entries)
    {
        const char* path = udev_list_entry_get_name(dev_list_entry);      
        
        dev = udev_device_new_from_syspath(udev, path);
        
        const char* bus = udev_device_get_property_value(dev, "ID_BUS");
        
        if (!strncmp(bus, "ata", 3))
        {
            const char *dev_path = udev_device_get_devnode(dev);
            
            parent = udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");
            if (!parent)
            {
                printf("dev_path=%s\n", dev_path);
            }
        }
    }

    udev_enumerate_unref(enumerate);
    
    udev_unref(udev);
}

int main()
{
    enumerate_sata_disk();
    
    return 0;
}
