#ifndef _PCI_H_
#define _PCI_H_

#include <stdint.h>

// PCI Configuration Space Access Mechanism #1 I/O Ports
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// PCI Configuration Space Register Offsets (Type 0x0)
#define PCI_VENDOR_ID       0x00
#define PCI_DEVICE_ID       0x02
#define PCI_COMMAND         0x04
#define PCI_STATUS          0x06
#define PCI_REVISION_ID     0x08
#define PCI_PROG_IF         0x09
#define PCI_SUBCLASS        0x0A
#define PCI_CLASS           0x0B
#define PCI_CACHE_LINE_SIZE 0x0C
#define PCI_LATENCY_TIMER   0x0D
#define PCI_HEADER_TYPE     0x0E
#define PCI_BIST            0x0F
#define PCI_BAR0            0x10
#define PCI_BAR1            0x14
#define PCI_BAR2            0x18
#define PCI_BAR3            0x1C
#define PCI_BAR4            0x20
#define PCI_BAR5            0x24
#define PCI_CAPABILITIES_PTR 0x34
#define PCI_INTERRUPT_LINE  0x3C
#define PCI_INTERRUPT_PIN   0x3D

// PCI Command Register bits
#define PCI_COMMAND_IO_SPACE        (1 << 0)
#define PCI_COMMAND_MEMORY_SPACE    (1 << 1)
#define PCI_COMMAND_BUS_MASTER      (1 << 2)

#define PCI_INTERRUPT_PIN   0x3D

// PCI Header Type field values
#define PCI_HEADER_TYPE_NORMAL 0x00

// Structure to hold PCI device address components
typedef struct {
    uint32_t bus;
    uint32_t device;
    uint32_t function;
} pci_device_address_t;

// Structure to hold general PCI device information
typedef struct pci_device {
    pci_device_address_t addr;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar[6]; // Base Address Registers
    uint8_t interrupt_line;
    uint8_t interrupt_pin;

    // Pointer to the next device in a linked list
    struct pci_device *next;
} pci_device_t;

/**
 * pci_config_read_dword() - Reads a 32-bit value from PCI configuration space.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 *
 * Returns the 32-bit value read from the specified PCI configuration register.
 */
uint32_t pci_config_read_dword(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg);

/**
 * pci_config_write_dword() - Writes a 32-bit value to PCI configuration space.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 * @value: The 32-bit value to write.
 */
void pci_config_write_dword(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg, uint32_t value);

/**
 * pci_config_read_word() - Reads a 16-bit value from PCI configuration space.
 * @bus: PCI bus number.
 * @device: PCI device number.
 * @function: PCI function number.
 * @reg: Register offset within the configuration space.
 *
 * Returns the 16-bit value read from the specified PCI configuration register.
 */
uint16_t pci_config_read_word(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg);

void pci_config_write_word(uint32_t bus, uint32_t device, uint32_t function, uint32_t reg, uint16_t value);

/**
 * pci_check_all_buses() - Scans all PCI buses, devices, and functions to enumerate devices.
 *                         Prints information about found devices (Vendor ID, Device ID, Class, Subclass).
 */
void pci_check_all_buses(void);

/**
 * pci_get_device_list() - Returns a pointer to the head of the linked list of discovered PCI devices.
 *
 * Returns a pci_device_t pointer to the first device in the list, or NULL if no devices were found.
 */
pci_device_t* pci_get_device_list(void);

/**
 * pci_get_device_by_class() - Searches the discovered PCI devices for a device with a matching class code.
 * @class_code: The class code to search for.
 * @subclass: The subclass to search for.
 * @prog_if: The programming interface byte to search for.
 * @device_index: Which instance of the device to return (0 for first, 1 for second, etc.)
 *
 * Returns a pointer to the pci_device_t if found, or NULL if no matching device is present.
 */
pci_device_t* pci_get_device_by_class(uint8_t class_code, uint8_t subclass, uint8_t prog_if, uint32_t device_index);

/**
 * pci_enable_bus_mastering() - Enables bus mastering for a given PCI device.
 * @device: Pointer to the pci_device_t structure.
 */
void pci_enable_bus_mastering(pci_device_t *device);

/**
 * pci_enable_memory_space() - Enables memory space access for a given PCI device.
 * @device: Pointer to the pci_device_t structure.
 */
void pci_enable_memory_space(pci_device_t *device);

/**
 * pci_enable_io_space() - Enables I/O space access for a given PCI device.
 * @device: Pointer to the pci_device_t structure.
 */
void pci_enable_io_space(pci_device_t *device);

#endif
