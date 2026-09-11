// NOTE: Literally an exact duplicate of 

/* symbols just to satisfy the linker*/
#define TEXT_STUB(name) void name(void) {}
#define DATA_STUB(name) int name;

TEXT_STUB(_stack_addr)
TEXT_STUB(_stack_end)

int MSL_TrigF_80400770[] = { 0x7FFFFFFF };
int MSL_TrigF_80400774[] = { 0x7F800000 };
