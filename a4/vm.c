// clang-format off

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm_dbg.h"

#define NOPS (16)

#define OPC(i) ((i) >> 12)
#define DR(i) (((i) >> 9) & 0x7)
#define SR1(i) (((i) >> 6) & 0x7)
#define SR2(i) ((i) & 0x7)
#define FIMM(i) ((i >> 5) & 01)
#define IMM(i) ((i) & 0x1F)
#define SEXTIMM(i) sext(IMM(i), 5)
#define FCND(i) (((i) >> 9) & 0x7)
#define POFF(i) sext((i) & 0x3F, 6)
#define POFF9(i) sext((i) & 0x1FF, 9)
#define POFF11(i) sext((i) & 0x7FF, 11)
#define FL(i) (((i) >> 11) & 1)
#define BR(i) (((i) >> 6) & 0x7)
#define TRP(i) ((i) & 0xFF)

/* New OS declarations */

// OS bookkeeping constants
#define PAGE_SIZE (4096)	// Page size in bytes
#define OS_MEM_SIZE (2)	 // OS Region size. Also the start of the page tables' page
#define Cur_Proc_ID (0)	 // id of the current process
#define Proc_Count (1)	 // total number of processes, including ones that finished executing.
#define OS_STATUS (2)				// Bit 0 shows whether the PCB list is full or not
#define OS_FREE_BITMAP (3)	// Bitmap for free pages

// Process list and PCB related constants
#define PCB_SIZE (3)	// Number of fields in a PCB
#define PID_PCB (0)		// Holds the pid for a process
#define PC_PCB (1)		// Value of the program counter for the process
#define PTBR_PCB (2)	// Page table base register for the process

#define CODE_SIZE (2)				// Number of pages for the code segment
#define HEAP_INIT_SIZE (2)	// Number of pages for the heap segment initially

bool running = true;

typedef void (*op_ex_f)(uint16_t i);
typedef void (*trp_ex_f)();

enum { trp_offset = 0x20 };
enum regist { R0 = 0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, PTBR, RCNT };
enum flags { FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2 };

uint16_t mem[UINT16_MAX] = { 0 };
uint16_t reg[RCNT] = { 0 };
uint16_t PC_START = 0x3000;

void initOS();
int createProc(char *fname, char *hname);
void loadProc(uint16_t pid);
uint16_t allocMem(
	uint16_t ptbr,
	uint16_t vpn,
	uint16_t read,
	uint16_t write
);	// Can use 'bool' instead
int freeMem(uint16_t ptr, uint16_t ptbr);
static inline uint16_t mr(uint16_t address);
static inline void mw(uint16_t address, uint16_t val);
static inline void tbrk();
static inline void thalt();
static inline void tyld();
static inline void trap(uint16_t i);

static inline uint16_t sext(uint16_t n, int b) {
	return ((n >> (b - 1)) & 1) ? (n | (0xFFFF << b)) : n;
}

static inline void uf(enum regist r) {
	if (reg[r] == 0) reg[RCND] = FZ;
	else if (reg[r] >> 15) reg[RCND] = FN;
	else reg[RCND] = FP;
}

static inline void add(uint16_t i) {
	reg[DR(i)] = reg[SR1(i)] + (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]);
	uf(DR(i));
}

static inline void and(uint16_t i) {
	reg[DR(i)] = reg[SR1(i)] & (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]);
	uf(DR(i));
}

static inline void ldi(uint16_t i) {
	reg[DR(i)] = mr(mr(reg[RPC] + POFF9(i)));
	uf(DR(i));
}

static inline void not(uint16_t i) {
	reg[DR(i)] = ~reg[SR1(i)];
	uf(DR(i));
}

static inline void br(uint16_t i) {
	if (reg[RCND] & FCND(i)) {
		reg[RPC] += POFF9(i);
	}
}

static inline void jsr(uint16_t i) {
	reg[R7] = reg[RPC];
	reg[RPC] = (FL(i)) ? reg[RPC] + POFF11(i) : reg[BR(i)];
}

static inline void jmp(uint16_t i) {
	reg[RPC] = reg[BR(i)];
}

static inline void ld(uint16_t i) {
	reg[DR(i)] = mr(reg[RPC] + POFF9(i));
	uf(DR(i));
}

static inline void ldr(uint16_t i) {
	reg[DR(i)] = mr(reg[SR1(i)] + POFF(i));
	uf(DR(i));
}

static inline void lea(uint16_t i) {
	reg[DR(i)] = reg[RPC] + POFF9(i);
	uf(DR(i));
}

static inline void st(uint16_t i) {
	mw(reg[RPC] + POFF9(i), reg[DR(i)]);
}

static inline void sti(uint16_t i) {
	mw(mr(reg[RPC] + POFF9(i)), reg[DR(i)]);
}

static inline void str(uint16_t i) {
	mw(reg[SR1(i)] + POFF(i), reg[DR(i)]);
}

static inline void rti(uint16_t i) {}	 // unused

static inline void res(uint16_t i) {}	 // unused

static inline void tgetc() {
	reg[R0] = getchar();
}

static inline void tout() {
	fprintf(stdout, "%c", (char) reg[R0]);
}

static inline void tputs() {
	uint16_t *p = mem + reg[R0];
	while (*p) {
		fprintf(stdout, "%c", (char) *p);
		p++;
	}
}

static inline void tin() {
	reg[R0] = getchar();
	fprintf(stdout, "%c", reg[R0]);
}

static inline void tputsp() { /* Not Implemented */ }

static inline void tinu16() {
	fscanf(stdin, "%hu", &reg[R0]);
}

static inline void toutu16() {
	fprintf(stdout, "%hu\n", reg[R0]);
}

trp_ex_f trp_ex[10] = { tgetc, tout,	 tputs,		tin,	tputsp,
												thalt, tinu16, toutu16, tyld, tbrk };

static inline void trap(uint16_t i) {
	trp_ex[TRP(i) - trp_offset]();
}

op_ex_f op_ex[NOPS] = { /*0*/ br, add, ld,	st,	 jsr, and, ldr, str,
												rti,			not, ldi, sti, jmp, res, lea, trap };

/**
  * Load an image file into memory.
  * @param fname the name of the file to load
  * @param offsets the offsets into memory to load the file
  * @param size the size of the file to load
*/
void ld_img(char *fname, uint16_t *offsets, uint16_t size) {
	FILE *in = fopen(fname, "rb");
	if (NULL == in) {
		fprintf(stderr, "Cannot open file %s.\n", fname);
		exit(1);
	}

	for (uint16_t s = 0; s < size; s += PAGE_SIZE) {
		uint16_t *p = mem + offsets[s / PAGE_SIZE];
		uint16_t writeSize = (size - s) > PAGE_SIZE ? PAGE_SIZE : (size - s);
		fread(p, sizeof(uint16_t), (writeSize), in);
	}

	fclose(in);
}

void run(char *code, char *heap) {
	while (running) {
		uint16_t i = mr(reg[RPC]++);
		op_ex[OPC(i)](i);
	}
}

// YOUR CODE STARTS HERE
// clang-format on
#define PCBS_START (12)

// uint16_max+1 is the number of distinct uint16 values, which means division results in n-1
#define PAGECOUNT ((UINT16_MAX / (PAGE_SIZE / 2)) + 1)

#define WRITE_PTE (1 << 2)
#define READ_PTE (1 << 1)
#define VALID_PTE (1 << 0)

// helper functions
static inline uint16_t extract_vpn(uint16_t virtual_address);
static inline uint16_t extract_offset(uint16_t virtual_address);
static inline uint16_t make_physical_address(uint16_t pte, uint16_t offset);
static inline uint16_t *pcb_address(uint16_t pid);
static uint16_t inline find_next_pid();

static inline void
decide_bit_position(uint16_t index, uint16_t *wordptr, uint16_t *mask) {
	if (index >= 32) {
		fprintf(
			stdout, "Bad call to get_bitmap_bit: index=%d out of range.", index
		);
		exit(1);
	}

	if (index < 16) {
		// first word.
		*wordptr = OS_FREE_BITMAP;
		*mask = 1 << (15 - index);
	} else {
		// second word
		*wordptr = OS_FREE_BITMAP + 1;
		*mask = 1 << (31 - index);
	}
}

// true if bit at index is 1, false if 0
static inline bool get_bitmap_bit(uint16_t index) {
	uint16_t wordptr, mask;
	decide_bit_position(index, &wordptr, &mask);
	return (mem[wordptr] & mask) != 0;
}

static inline void set_bitmap_bit(uint16_t index, bool set) {
	uint16_t wordptr, mask;
	decide_bit_position(index, &wordptr, &mask);
	if (set) mem[wordptr] = (mem[wordptr] | mask);
	else mem[wordptr] = mem[wordptr] & (~mask);
	return;
}

// returns 32 if not
static inline uint16_t find_unset_bitmap_bit() {
	uint16_t pfn;
	// skip first 3 because they should be reserved for os.
	for (pfn = 3; pfn < (PAGECOUNT); pfn++) {
		if (get_bitmap_bit(pfn)) break;	 // find first 1
	}

	return pfn;
}

inline static uint16_t *pcb_address(uint16_t pid) {
	return mem + PCBS_START + (pid * PCB_SIZE);
}

void initOS() {
	mem[Cur_Proc_ID] = UINT16_MAX;
	mem[Proc_Count] = 0;
	mem[OS_STATUS] = 0x0000;

	// starts with pages 1, 2, 3 reserved for OS
	mem[OS_FREE_BITMAP] = 0xffff;
	mem[OS_FREE_BITMAP + 1] = 0xffff;
	set_bitmap_bit(0, false);
	set_bitmap_bit(1, false);
	set_bitmap_bit(2, false);

	return;
}

// Process functions to implement
int createProc(char *fname, char *hname) {
	if ((mem[OS_STATUS] & 0x0001) != 0x0000) {
		fprintf(stdout, "The OS memory region is full. Cannot create a new PCB.\n");
		return 0;
	}

	uint16_t ptbr = (OS_MEM_SIZE * PAGE_SIZE / 2) + (PAGECOUNT * mem[Proc_Count]);
	// if ptbr > end of pt page - 2 page tables, there is no space for another page table.
	if (ptbr >= (((OS_MEM_SIZE + 1) * 2048) - (2 * (PAGECOUNT))))
		mem[OS_STATUS] = 1;

	// allocate some memory for code and initial heap.
	uint16_t page_offsets[CODE_SIZE + HEAP_INIT_SIZE];
	for (size_t i = 0; i < (CODE_SIZE + HEAP_INIT_SIZE); i++) {
		page_offsets[i] = (PAGE_SIZE / 2)
										* allocMem(
												ptbr,
												i + (0x3000 / (PAGE_SIZE / 2)),
												UINT16_MAX,
												i < 2 ? 0x0000 : UINT16_MAX
										);

		// if there is no space to allocate, undo and exit
		if (page_offsets[i] == 0) {
			// deallocate the first i pages
			for (size_t j = 0; j < i; j++) {
				freeMem(i + 3, ptbr);
			}

			fprintf(stdout, "Cannot create %s segment.", i < 2 ? "code" : "heap");
			return 0;
		}
	}

	// create pcb
	uint16_t *pcb = pcb_address(mem[Proc_Count]);
	pcb[PID_PCB] = mem[Proc_Count];
	pcb[PC_PCB] = PC_START;
	pcb[PTBR_PCB] = ptbr;

	mem[Proc_Count]++;

	// load code and heap
	ld_img(fname, page_offsets + 0, PAGE_SIZE * 2);
	ld_img(hname, page_offsets + 2, PAGE_SIZE * 2);

	// check if next pcb would overflow
	if ((PCBS_START + (mem[Cur_Proc_ID] * PCB_SIZE) + 2)	// next pcb's end
			>																									// is at or after
			(OS_MEM_SIZE * (PAGE_SIZE / 2))	 // first address after pcb table region
	)
		mem[OS_STATUS] = 1;


	// check if next page table would overflow

	return 1;
}

void loadProc(uint16_t pid) {
	uint16_t *pcb = pcb_address(pid);
	if (pcb[PID_PCB] != pid) exit(1);

	reg[RPC] = pcb[PC_PCB];
	reg[PTBR] = pcb[PTBR_PCB];
	mem[Cur_Proc_ID] = pid;

	return;
}

// linear search unallocated spot, create correct pte, write pte.
uint16_t allocMem(uint16_t ptbr, uint16_t vpn, uint16_t read, uint16_t write) {
	uint16_t pfn = find_unset_bitmap_bit();

	// if not found
	if (pfn == 32) return 0;

	// if already allocated
	if ((mem[ptbr + vpn] & VALID_PTE) != 0) return 0;

	// write pte
	mem[ptbr + vpn] =
		(pfn << 11) | (read & READ_PTE) | (write & WRITE_PTE) | VALID_PTE;
	set_bitmap_bit(pfn, false);

	return pfn;
}

int freeMem(uint16_t vpn, uint16_t ptbr) {
	uint16_t pte = mem[ptbr + vpn];
	if ((pte & VALID_PTE) == 0) return 0;	 // not allocated
	else {
		// set bit on bitmap
		set_bitmap_bit(((pte >> 11) & 0x001f), true);
		// unset valid bit on pte (must be set from above "if")
		mem[ptbr + vpn] = pte ^ VALID_PTE;
	}
	return 0;
}

// find pid that hasn't terminated to switch to.
// if none found, returns 0xffff
static uint16_t inline find_next_pid() {	// save pc, ptbr.
	uint16_t current_pid = mem[Cur_Proc_ID];
	uint16_t proc_count = mem[Proc_Count];
	for (int i = 1; i <= proc_count; i++) {
		uint16_t next_pid = (current_pid + i) % proc_count;
		uint16_t *next_pcb = pcb_address(next_pid);
		if (next_pcb[PID_PCB] != 0xffff)	// found unterminated process
			return next_pid;
	}
	// no runnable processes.
	return 0xffff;
}

// Instructions to implement
static inline void tbrk() {
	uint16_t input = reg[R0];
	if ((input & VALID_PTE) != 0) {
		fprintf(
			stdout, "Heap increase requested by process %d.\n", mem[Cur_Proc_ID]
		);
		uint16_t vpn = extract_vpn(input);
		if ((mem[reg[PTBR] + vpn] & VALID_PTE) != 0) {	// already allocated
			fprintf(
				stdout,
				"Cannot allocate memory for page %d of pid %d since it is already "
				"allocated.",
				vpn,
				mem[Cur_Proc_ID]
			);
			return;
		}

		if (allocMem(reg[PTBR], vpn, input, input) == 0) {
			fprintf(
				stdout,
				"Cannot allocate more space for pid %d since there is no free page "
				"frames.",
				mem[Cur_Proc_ID]
			);
			return;
		}

	} else {
		fprintf(
			stdout, "Heap decrease requested by process %d.\n", mem[Cur_Proc_ID]
		);
		freeMem(extract_vpn(input), reg[PTBR]);
	}
}

static inline void tyld() {
	uint16_t current_pid = mem[Cur_Proc_ID];
	uint16_t *current_pcb = pcb_address(current_pid);
	uint16_t next_pid = find_next_pid();

	// do nothing if we want to continue executing
	if (next_pid == current_pid) return;

	// save current pid
	current_pcb[PC_PCB] = reg[RPC];
	current_pcb[PTBR_PCB] = reg[PTBR];

	// load next
	loadProc(next_pid);

	fprintf(
		stdout, "We are switching from process %d to %d.\n", current_pid, next_pid
	);
	return;
}

// Instructions to modify
static inline void thalt() {
	// set this process as terminated
	pcb_address(mem[Cur_Proc_ID])[PID_PCB] = 0xffff;

	// free all pages.
	for (uint16_t i = 0; i < PAGECOUNT; i++) {
		freeMem(i, reg[PTBR]);
	}

	uint16_t next_pid = find_next_pid();
	if (next_pid == 0xffff) running = false;
	// do a context switch.
	else return loadProc(next_pid);
}

static inline uint16_t extract_vpn(uint16_t virtual_address) {
	// take 5 MSB and make into number
	return (virtual_address >> 11) & 0x001f;
}

static inline uint16_t extract_offset(uint16_t virtual_address) {
	// drop 5 MSB
	return virtual_address & 0x07ff;
}

// first 5 bits of offset are IGNORED
static inline uint16_t make_physical_address(uint16_t pte, uint16_t offset) {
	return extract_offset(offset) | (pte & 0xf800);
}

static inline uint16_t
translate_address(uint16_t address, bool write_required) {
	uint16_t vpn = extract_vpn(address);

	// reserved space
	if (vpn < 3) {
		fprintf(stdout, "Segmentation fault.\n");
		exit(1);
	}

	uint16_t offset = extract_offset(address);	// right 11
	uint16_t pte = mem[reg[PTBR] + vpn];

	if ((pte & VALID_PTE) == 0) {
		fprintf(stdout, "Segmentation fault inside free space.\n");
		exit(1);
	}

	if (write_required && ((pte & WRITE_PTE) == 0)) {
		fprintf(stdout, "Cannot write to a read-only page.\n");
		exit(1);
	}

	return make_physical_address(pte, offset);
}

static inline uint16_t mr(uint16_t address) {
	return mem[translate_address(address, false)];
}

static inline void mw(uint16_t address, uint16_t val) {
	mem[translate_address(address, true)] = val;
	return;
}

// YOUR CODE ENDS HERE
