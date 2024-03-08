#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1;  // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count) {
    int pt_page = -1;

    // Allocate a single page for this process's page table
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(0, i);
        if (mem[addr] == 0) { // Page is free
            pt_page = i;
            mem[addr] = 1; // Mark page as allocated
            break;
        }
    }

    if (pt_page == -1) {
        printf("OOM: proc %d: page table\n", proc_num);
        return;
    }

    // Allocate the data pages the process requested
    int data_pages[page_count];
    memset(data_pages, -1, sizeof(data_pages)); // Initialize data_pages with -1

    for (int j = 0; j < page_count; j++) {
        int allocated = -1;
        for (int i = 0; i < PAGE_COUNT; i++) {
            int addr = get_address(0, i);
            if (mem[addr] == 0) { // Page is free
                allocated = i;
                mem[addr] = 1; // Mark page as allocated
                data_pages[j] = allocated;
                break;
            }
        }

        if (allocated == -1) { // Check after trying to allocate each data page
            printf("OOM: proc %d: data page\n", proc_num);
            return; // Exiting the function if OOM occurs
        }
    }

    // Set the page table pointer
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    mem[ptp_addr] = pt_page;

    // Set the page table entries
    int pt_addr = get_address(pt_page, 0);
    for (int i = 0; i < page_count; i++) {
        if (data_pages[i] != -1) { // Ensure the page was allocated
            mem[pt_addr + i] = data_pages[i];
        }
    }
}

//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++) {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }
    
    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "pfm") == 0) {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0) {
            int proc_num = atoi(argv[++i]);
            int page_count = atoi(argv[++i]);
            
            new_process(proc_num, page_count);
        }
    }
}
