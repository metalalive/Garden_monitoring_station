#include "station_include.h"

// brief This function handles Non maskable interrupt.
void NMI_Handler(void) { XASSERT(0); }

// brief This function handles Debug monitor.
void DebugMon_Handler(void) { XASSERT(0); }

// brief This function handles Pre-fetch fault, memory access fault.
void BusFault_Handler(void) { XASSERT(0); }

// brief This function handles Undefined instruction or illegal state.
void UsageFault_Handler(void) { XASSERT(0); }

void vAppHardFaultHandler(UBaseType_t *sp) {
    BaseType_t done = vPortTryRecoverHardFault(sp);
    XASSERT(done);
}
