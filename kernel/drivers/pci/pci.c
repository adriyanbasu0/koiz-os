#include <stddef.h>
#include "pci.h"
#include "../io/basic_io.h"
#include "../../libc/stdlib.h" // For printf and panic
#include "../../libc/malloc.h" // For kmalloc

#define PCI_CONFIG_ENABLE 0x80000000
#define PCI_MAX_BUSES       256
#define PCI_MAX_DEVICES     32
#define PCI_MAX_FUNCTIONS   8

// Vendor ID that indicates no device is present
#define PCI_VENDOR_ID_INVALID 0xFFFF

// Global linked list head for PCI devices
static pci_device_t *pci_devices_head = NULL;

/**
 * pci_config_address_make() - Creates the 32-bit configuration address.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 *
 * Returns the constructed 32-bit address suitable for writing to PCI_CONFIG_ADDRESS.
 */
static uint32_t pci_config_address_make(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg)
{
    uint32_t address;
    address = (uint32_t)((bus << 16) |
                         (device << 11) |
                         (function << 8) |
                         (reg & 0xFC) |
                         PCI_CONFIG_ENABLE);
    return address;
}

/**
 * pci_config_read_dword() - Reads a 32-bit value from PCI configuration space.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 *
 * Returns the 32-bit value read from the specified PCI configuration register.
 */
uint32_t pci_config_read_dword(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg)
{
        uint32_t address = pci_config_address_make(bus, device, function, reg);
    io_dword_out(PCI_CONFIG_ADDRESS, address);
    return io_dword_in(PCI_CONFIG_DATA);
}

/**
 * pci_config_write_dword() - Writes a 32-bit value to PCI configuration space.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 * @value: The 32-bit value to write.
 */
void pci_config_write_dword(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg, uint32_t value)
{
    uint32_t address = pci_config_address_make(bus, device, function, reg);
    io_dword_out(PCI_CONFIG_ADDRESS, address);
    io_dword_out(PCI_CONFIG_DATA, value);
}

/**
 * pci_config_read_word() - Reads a 16-bit value from PCI configuration space.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 *
 * Returns the 16-bit value read from the specified PCI configuration register.
 */
uint16_t pci_config_read_word(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg)
{
    uint32_t address = pci_config_address_make(bus, device, function, reg);
    io_dword_out(PCI_CONFIG_ADDRESS, address);
    // Read the 32-bit value and extract the 16-bit word based on the register offset
    uint32_t dword = io_dword_in(PCI_CONFIG_DATA);
    return (uint16_t)((dword >> ((reg % 4) * 8)) & 0xFFFF);
}

/**
 * pci_config_write_word() - Writes a 16-bit value to PCI configuration space.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 * @value: The 16-bit value to write.
 */
void pci_config_write_word(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg, uint16_t value)
{
    uint32_t address = pci_config_address_make(bus, device, function, reg);
    io_dword_out(PCI_CONFIG_ADDRESS, address);

    // Read the current 32-bit dword, modify the relevant 16 bits, and write it back
    uint32_t dword = io_dword_in(PCI_CONFIG_DATA);
    uint32_t shift = (reg % 4) * 8;
    dword &= ~(0xFFFF << shift); // Clear the old 16-bit value
    dword |= ((uint32_t)value << shift);   // Set the new 16-bit value
    io_dword_out(PCI_CONFIG_DATA, dword);
}

/**
 * pci_check_device() - Checks a specific PCI device and function.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 *
 * If a valid device is found, allocates a pci_device_t, populates it,
 * and adds it to the global linked list. Prints information about found devices.
 * Returns a pointer to the newly created pci_device_t, or NULL if no device is found.
 */
static pci_device_t* pci_check_device(uint32_t bus, uint32_t device, uint32_t function)
{
    uint16_t vendor_id = pci_config_read_word(bus, device, function, PCI_VENDOR_ID);

    if (vendor_id == PCI_VENDOR_ID_INVALID) {
        return NULL; // No device at this location
    }

    pci_device_t *new_device = (pci_device_t*)malloc(sizeof(pci_device_t));
    if (new_device == NULL) {
        panic("pci_check_device: Failed to allocate memory for PCI device!");
    }

    new_device->addr.bus = bus;
    new_device->addr.device = device;
    new_device->addr.function = function;
    new_device->vendor_id = vendor_id;
    new_device->device_id = pci_config_read_word(bus, device, function, PCI_DEVICE_ID);
    new_device->command = pci_config_read_word(bus, device, function, PCI_COMMAND);
    new_device->status = pci_config_read_word(bus, device, function, PCI_STATUS);
    new_device->revision_id = (uint8_t)pci_config_read_word(bus, device, function, PCI_REVISION_ID);
    new_device->prog_if = (uint8_t)pci_config_read_word(bus, device, function, PCI_PROG_IF);
    new_device->subclass = (uint8_t)pci_config_read_word(bus, device, function, PCI_SUBCLASS);
    new_device->class_code = (uint8_t)pci_config_read_word(bus, device, function, PCI_CLASS);
    new_device->cache_line_size = (uint8_t)pci_config_read_word(bus, device, function, PCI_CACHE_LINE_SIZE);
    new_device->latency_timer = (uint8_t)pci_config_read_word(bus, device, function, PCI_LATENCY_TIMER);
    new_device->header_type = (uint8_t)pci_config_read_word(bus, device, function, PCI_HEADER_TYPE);
    new_device->bist = (uint8_t)pci_config_read_word(bus, device, function, PCI_BIST);

    for (int i = 0; i < 6; i++) {
        new_device->bar[i] = pci_config_read_dword(bus, device, function, PCI_BAR0 + (i * 4));
    }

    new_device->interrupt_line = (uint8_t)pci_config_read_word(bus, device, function, PCI_INTERRUPT_LINE);
    new_device->interrupt_pin = (uint8_t)pci_config_read_word(bus, device, function, PCI_INTERRUPT_PIN);
    new_device->next = NULL;

    // Add to linked list
    if (pci_devices_head == NULL) {
        pci_devices_head = new_device;
    } else {
        pci_device_t *current = pci_devices_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_device;
    }

    printf("PCI Device Found: Bus %x, Device %x, Function %x - Vendor ID: %x, Device ID: %x, Class: %x, Subclass: %x\n",
           bus, device, function, new_device->vendor_id, new_device->device_id, new_device->class_code, new_device->subclass);

    return new_device;
}

/**
 * pci_check_function() - Checks a specific PCI function and handles multi-function devices.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 *
 * Calls pci_check_device. If the device is a multi-function device, it will iterate through
 * all functions of that device.
 */
static void pci_check_function(uint32_t bus, uint32_t device, uint32_t function)
{
    pci_device_t *dev_found = pci_check_device(bus, device, function);
    if (dev_found == NULL) {
        return; // No device here
    }

    // Check if multi-function device (only for function 0)
    if (function == 0 && (dev_found->header_type & 0x80) != 0) {
        for (uint32_t func = 1; func < PCI_MAX_FUNCTIONS; func++) {
            pci_check_device(bus, device, func);
        }
    }
}

/**
 * pci_check_device_and_functions() - Checks a specific PCI device and all its functions.
 * @bus: PCI bus number.
 * @device: PCI device number.
 *
 * Calls pci_check_function for the primary function (0), which then handles
 * multi-function devices by iterating through other functions if necessary.
 */
static void pci_check_device_and_functions(uint32_t bus, uint32_t device)
{
    pci_check_function(bus, device, 0); // Start with function 0
}

/**
 * pci_check_all_buses() - Scans all PCI buses, devices, and functions to enumerate devices.
 *                         Prints information about found devices (Vendor ID, Device ID, Class, Subclass).
 */
void pci_check_all_buses(void)
{
    printf("Scanning PCI buses for devices...\n");
    for (uint32_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
        for (uint32_t device = 0; device < PCI_MAX_DEVICES; device++) {
            pci_check_device_and_functions(bus, device);
        }
    }
    printf("PCI scan complete.\n");
}

/**
 * pci_get_device_list() - Returns a pointer to the head of the linked list of discovered PCI devices.
 *
 * Returns a pci_device_t pointer to the first device in the list, or NULL if no devices were found.
 */
pci_device_t* pci_get_device_list(void) {
    return pci_devices_head;
}

/**
 * pci_get_device_by_class() - Searches the discovered PCI devices for a device with a matching class code.
 * @class_code: The class code to search for.
 * @subclass: The subclass to search for.
 * @prog_if: The programming interface byte to search for.
 * @device_index: Which instance of the device to return (0 for first, 1 for second, etc.)
 *
 * Returns a pointer to the pci_device_t if found, or NULL if no matching device is present.
 */
pci_device_t* pci_get_device_by_class(uint8_t class_code, uint8_t subclass, uint8_t prog_if, uint32_t device_index) {
    pci_device_t *current = pci_devices_head;
    uint32_t current_index = 0;

    while (current != NULL) {
        // The class_code, subclass, and prog_if are 8-bit values.
        // The pci_device_t stores them as uint8_t directly.
        // No shifting or masking is needed for comparison if they are stored correctly.
        if (current->class_code == class_code &&
            current->subclass == subclass &&
            current->prog_if == prog_if) {
            if (current_index == device_index) {
                return current;
            }
            current_index++;
        }
        current = current->next;
    }

    return NULL; // Device not found
}

/**
 * pci_enable_bus_mastering() - Enables bus mastering for a given PCI device.
 * @device: Pointer to the pci_device_t structure.
 */
void pci_enable_bus_mastering(pci_device_t *device) {
    if (device == NULL) {
        return;
    }
    uint16_t command = pci_config_read_word(device->addr.bus, device->addr.device, device->addr.function, PCI_COMMAND);
    command |= PCI_COMMAND_BUS_MASTER;
    pci_config_write_word(device->addr.bus, device->addr.device, device->addr.function, PCI_COMMAND, command);
    device->command = command; // Update the cached command register value
    printf("PCI: Enabled Bus Mastering for device B:%x D:%x F:%x\n", device->addr.bus, device->addr.device, device->addr.function);
}

/**
 * pci_enable_memory_space() - Enables memory space access for a given PCI device.
 * @device: Pointer to the pci_device_t structure.
 */
void pci_enable_memory_space(pci_device_t *device) {
    if (device == NULL) {
        return;
    }
    uint16_t command = pci_config_read_word(device->addr.bus, device->addr.device, device->addr.function, PCI_COMMAND);
    command |= PCI_COMMAND_MEMORY_SPACE;
    pci_config_write_word(device->addr.bus, device->addr.device, device->addr.function, PCI_COMMAND, command);
    device->command = command; // Update the cached command register value
    printf("PCI: Enabled Memory Space for device B:%x D:%x F:%x\n", device->addr.bus, device->addr.device, device->addr.function);
}

/**
 * pci_enable_io_space() - Enables I/O space access for a given PCI device.
 * @device: Pointer to the pci_device_t structure.
 */
void pci_enable_io_space(pci_device_t *device) {
    if (device == NULL) {
        return;
    }
    uint16_t command = pci_config_read_word(device->addr.bus, device->addr.device, device->addr.function, PCI_COMMAND);
    command |= PCI_COMMAND_IO_SPACE;
    pci_config_write_word(device->addr.bus, device->addr.device, device->addr.function, PCI_COMMAND, command);
    device->command = command; // Update the cached command register value
    printf("PCI: Enabled I/O Space for device B:%x D:%x F:%x\n", device->addr.bus, device->addr.device, device->addr.function);
}