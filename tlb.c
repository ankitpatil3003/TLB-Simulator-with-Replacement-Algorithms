#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char ADDRESS_SPACE_SIZE = 16; // 16-bit address space
char *PAGE_SIZE_B = NULL;     // Page size in bytes
short unsigned int *PPN = NULL;        // Physical Page Number
short unsigned int *NEW_ADDRESS = NULL; // New Virtual Address to service
short unsigned int *TLB_SIZE = NULL;    // Translation Lookaside Buffer entries
unsigned char REPL;            // 0 for FIFO, 1 for Random, 2 for Clock
int seed;

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4)
    {
        printf("You must enter an input file to test and a replacement policy!\n");
        exit(-1);
    }

    FILE *fp;
    PAGE_SIZE_B = malloc(1);
    NEW_ADDRESS = malloc(2);
    PPN = malloc(2);
    TLB_SIZE = malloc(1);

    fp = fopen(argv[1], "rb");
    REPL = atoi(argv[2]);
    if (argc == 4)
    {
        seed = atoi(argv[3]);
        srand(seed);
    }
    fread(PAGE_SIZE_B, 1, 1, fp);     // The first byte of the binary will be the page size
    fread(TLB_SIZE, 1, 1, fp);        // The second byte of the binary will be the TLB size
    short int PAGE_TABLE_ENTRIES = pow(2, ADDRESS_SPACE_SIZE - *PAGE_SIZE_B); // Calculate the number of Page Table Entries
    // Iterate 2 bytes at a time the correct number of PTEs to get the physical address mappings
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    {
        if (fread(PPN, 2, 1, fp) == 1)
        {
            // Here we read 1 (2 byte) Physical Address at a time; you must map it to the corresponding Page Table Entry
        }
        else
        {
            printf("Something went wrong with fread!\n");
            exit(-1);
        }
    }

    int TLB_SIZE_VAL = *TLB_SIZE;
    int TLB[TLB_SIZE_VAL][2]; // TLB structure: [Virtual Page Number, Physical Page Number]
    int TLB_USED[TLB_SIZE_VAL]; // Keeps track of which TLB entries are being used

    int PAGE_FAULTS = 0;
    int TLB_HITS = 0;
    int TLB_MISSES = 0;

    int CLOCK_HAND = 0;

    for (int i = 0; i < TLB_SIZE_VAL; i++)
    {
        TLB_USED[i] = 0; // Initialize all TLB entries as unused
    }

    while (fread(NEW_ADDRESS, 2, 1, fp) == 1)
    {
        unsigned short VIRTUAL_ADDRESS = *NEW_ADDRESS;
        unsigned short VIRTUAL_PAGE_NUMBER = VIRTUAL_ADDRESS >> *PAGE_SIZE_B;
        unsigned short OFFSET = VIRTUAL_ADDRESS & ((1 << *PAGE_SIZE_B) - 1);
        int PHYSICAL_PAGE_NUMBER = -1;
        int TLB_INDEX = -1;
        int TLB_HIT_FLAG = 0;

        // Check if the translation is in the TLB
        for (int i = 0; i < TLB_SIZE_VAL; i++)
        {
            if (TLB_USED[i] && TLB[i][0] == VIRTUAL_PAGE_NUMBER)
            {
                PHYSICAL_PAGE_NUMBER = TLB[i][1];
                TLB_HIT_FLAG = 1;
                TLB_HITS++;
                break;
            }
        }

        if (PHYSICAL_PAGE_NUMBER == -1)
        {
            // TLB miss, perform page table lookup
            TLB_MISSES++;
            // Check if the translation is in the page table
            for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
            {
                if (PPN[i] == VIRTUAL_PAGE_NUMBER)
                {
                    PHYSICAL_PAGE_NUMBER = PPN[i]; // Set the physical page number
                    // Update TLB
                    if (REPL == 0 || REPL == 2)
                    {
                        // FIFO or Clock replacement
                        TLB[CLOCK_HAND][0] = VIRTUAL_PAGE_NUMBER;
                        TLB[CLOCK_HAND][1] = PHYSICAL_PAGE_NUMBER;
                        TLB_USED[CLOCK_HAND] = 1;
                        CLOCK_HAND = (CLOCK_HAND + 1) % TLB_SIZE_VAL;
                    }
                    else if (REPL == 1)
                    {
                        // Random replacement
                        int random_index = rand() % TLB_SIZE_VAL;
                        TLB[random_index][0] = VIRTUAL_PAGE_NUMBER;
                        TLB[random_index][1] = PHYSICAL_PAGE_NUMBER;
                        TLB_USED[random_index] = 1;
                    }
                    break;
                }
            }

            if (PHYSICAL_PAGE_NUMBER == -1)
            {
                // Page fault, handle accordingly
                PAGE_FAULTS++;
                continue;
            }
        }

        // Translate virtual address to physical address
        unsigned short TRANSLATED_PHYSICAL_ADDRESS = (PHYSICAL_PAGE_NUMBER << *PAGE_SIZE_B) + OFFSET;

        // Print the translation result
        printf("VA:%x -- PA:%x\n", VIRTUAL_ADDRESS, TRANSLATED_PHYSICAL_ADDRESS);
    }

    // Print page fault, TLB hit, and TLB miss statistics
    printf("Page Faults: %d\nTLB hits: %d\nTlb misses: %d\n", PAGE_FAULTS, TLB_HITS, TLB_MISSES);

    free(PAGE_SIZE_B);
    free(NEW_ADDRESS);
    free(PPN);
    free(TLB_SIZE);
    fclose(fp);

    return 0;
}
