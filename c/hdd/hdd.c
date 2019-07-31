#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <linux/hdreg.h>
#include <scsi/scsi.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define check_errno(err, msg)                                                                      \
if (err == -1)                                                                                     \
{                                                                                                  \
    printf("%s:%d (%s) %s: %s\n", __FILE__, __LINE__, __FUNCTION__, msg, strerror(errno));         \
    ret = -1;                                                                                      \
    goto exit;                                                                                     \
}

bool hdd_is_sata(char *device)
{
    bool ret = false;
    int rc = 0;
    int fd = -1;
    char buff[512] = {0};
    int bus_num;
    struct hd_driveid  driveid;
    
    fd = open(device, O_RDONLY | O_NONBLOCK);
    check_errno(fd, "Fail to open device");
    
    // HDIO_GET_IDENTITY fail on usb storage
    rc = ioctl(fd, HDIO_GET_IDENTITY, &driveid);
    if (rc == 0)
    {
        printf("Device %s is SATA\n", device);
        printf("    Serial number: %s\n", driveid.serial_no);
        printf("    Model: %s\n", driveid.model);
        ret = true;
    }
    
exit:
    if (fd)
        close(fd);
    
    return ret;
}


void list_disk()
{
    struct udev_list_entry *devices;
    struct udev_enumerate *enumerate;
    struct udev *udev;
    struct udev_device *dev;
    struct udev_list_entry *dev_list_entry;


    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
            char *path;
            path = udev_device_get_devnode(dev);
    }
    udev_enumerate_unref(enumerate);
}

int main()
{
    //hdd_is_sata("/dev/sda");
    list_disk();
}
