#include <algorithm>
#include <cstdio>
#include <string>

extern "C" {
#include <fcntl.h>  //needed for open
#include <unistd.h> //needed for close

#include <xf86drm.h>

#include <libudev.h>
#include <wlr/util/log.h>
}

#include "util.hpp"

static std::string *get_version_if_kms(const char *path) {
  std::string *ret;
  if (!path) {
    return nullptr;
  }

  int fd = open(path, 0);
  if (fd < 0) {
    return nullptr;
  }

  drmVersion *version = drmGetVersion(fd);
  close(fd);
  if (!version) {
    return nullptr;
  }
  ret = new std::string(version->name);
  drmFreeVersion(version);
  return ret;
}

bool possible_no_hardware_cursor_support() {
  const int gpus_num = 32;
  int gpus[gpus_num];
  auto my_udev = udev_new();
  if (!my_udev) {
    wlr_log_errno(WLR_ERROR, "Failed to create udev context");
  }
  struct udev_enumerate *en = udev_enumerate_new(my_udev);
  if (!en) {
    wlr_log(WLR_ERROR, "Failed to create udev enumeration");
    return false;
  }

  udev_enumerate_add_match_subsystem(en, "drm");
  udev_enumerate_add_match_sysname(en, "card[0-9]*");
  udev_enumerate_scan_devices(en);

  struct udev_list_entry *entry;
  size_t i = 0;

  udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(en)) {
    if (i == gpus_num) {
      break;
    }

    bool is_boot_vga = false;

    const char *path = udev_list_entry_get_name(entry);
    struct udev_device *dev = udev_device_new_from_syspath(my_udev, path);
    if (!dev) {
      continue;
    }

    // This is owned by 'dev', so we don't need to free it
    struct udev_device *pci =
        udev_device_get_parent_with_subsystem_devtype(dev, "pci", NULL);

    if (pci) {
      const char *id = udev_device_get_sysattr_value(pci, "boot_vga");
      if (id && strcmp(id, "1") == 0) {
        is_boot_vga = true;
      }
    }

    std::string *version_name =
        get_version_if_kms(udev_device_get_devnode(dev));
    udev_device_unref(dev);
    if (version_name == nullptr) {
      delete version_name;
      continue;
    }

    if (is_boot_vga) {
      if (std::find(drivers_wo_hw_cursor_support.begin(),
                    drivers_wo_hw_cursor_support.end(),
                    *version_name) != drivers_wo_hw_cursor_support.end()) {
        wlr_log(WLR_INFO,
                "Possible lack of support for hardware cursors on this driver: "
                "\"%s\"",
                (*version_name).c_str());
        wlr_log(WLR_INFO, "Falling back to software cursor");
        delete version_name;
        udev_enumerate_unref(en);
        udev_unref(my_udev);
        return true;
      }
    }
    delete version_name;
    ++i;
  }

  udev_enumerate_unref(en);
  udev_unref(my_udev);

  return false;
}