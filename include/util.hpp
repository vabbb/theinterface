#ifndef TI_UTIL_HPP
#define TI_UTIL_HPP

#include <string>
#include <vector>

// drivers possibly without hw cursor support
const std::vector<std::string> drivers_wo_hw_cursor_support{
    // used by both virtualbox and vmware
    // even though Virtualbox supports hw cursors, VMWare still doesnt
    "vmwgfx"};

/*! Checks every GPU it finds with the "boot_vga" attribute if the version
 * equals to vmwgfx, the drm driver for vmware vms
 * \return True if running in a VM
 */
bool possible_no_hardware_cursor_support();

#endif