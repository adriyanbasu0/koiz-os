#ifndef _VMEM_H_
#define _VMEM_H_

/* Page table flags */
#define VMEM_FLAG_PRESENT       0x1     // Page is present in memory
#define VMEM_FLAG_RW            0x2     // Read/Write
#define VMEM_FLAG_USER          0x4     // User/Supervisor
#define VMEM_FLAG_WT            0x8     // Write-through caching
#define VMEM_FLAG_CACHE_DISABLED 0x10    // Cache disabled
#define VMEM_FLAG_ACCESSED      0x20    // Page has been accessed
#define VMEM_FLAG_DIRTY         0x40    // Page has been written to
#define VMEM_FLAG_GLOBAL        0x100   // Global page (prevents TLB flush)

/**
 * vmem_init() - Initializes paging for the kernel
 * 
 * This will automatically populate the page directory structure
 * as well as the page tables and enable them for the kernel
 */
void vmem_init(void);

/**
 * vmem_map_page() - Maps a physical page to a virtual page
 *
 * @vaddr: Virtual address to map
 * @paddr: Physical address to map
 * @flags: Page flags (e.g., VMEM_FLAG_PRESENT, VMEM_FLAG_RW, VMEM_FLAG_CACHE_DISABLED)
 *
 * This function creates or updates a page table entry to map the given physical
 * address to the virtual address with the specified flags.
 */
void vmem_map_page(void* vaddr, void* paddr, uint32_t flags);

#endif