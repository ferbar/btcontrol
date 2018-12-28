#include <Arduino.h>
// für esp_stack_ptr_is_sane
#include <esp_panic.h>
#include "utils.h"

// https://github.com/espressif/esp-idf/blob/23b6d40c537bec674537d52fdea34857a4892dc9/components/esp32/panic.c
void putBacktraceEntryESP32(uint32_t pc, uint32_t sp) {
	if (pc & 0x80000000) {
		pc = (pc & 0x3fffffff) | 0x40000000;
	}
	Serial.printf("%#x:%#x ", pc, sp);
}

/**
 * kopiert von panic.c doBacktrace() - die funktion ist nicht public
 */
void utils::dumpBacktrace() {
	// backtrace:
	register uint32_t curr_sp asm ("sp");
	register uint32_t curr_pc asm ("a0");
	register uint32_t curr_a1 asm ("a1");
	uint32_t sp=curr_sp;
	uint32_t pc=curr_pc;

	Serial.println("for ESP Exception Decoder");
	Serial.print("Backtrace: ");
	// Serial.printf("%#x:%#x %#x issane:%d\n", sp, pc, curr_a1, esp_stack_ptr_is_sane(sp));
	if(!esp_stack_ptr_is_sane(sp)) {
		Serial.printf("invalid stack pointer\n");
		return;
	}
	putBacktraceEntryESP32(pc-3, *((uint32_t *) sp));
	int i=0;
	while (i++ < 100) {
		uint32_t psp = sp;
		if (!esp_stack_ptr_is_sane(sp) || i++ > 100) {
			break;
		}
		sp = *((uint32_t *) (sp - 0x10 + 4));
		// putEntry(pc - 3, sp); // stack frame addresses are return addresses, so subtract 3 to get the CALL address
		putBacktraceEntryESP32(pc-3, sp);
		pc = *((uint32_t *) (psp - 0x10));
		if (pc < 0x40000000) {
			break;
		}
	}
	Serial.println();
}
