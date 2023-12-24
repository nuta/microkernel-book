#pragma once

#define WASM_MAGIC      "\00asm"
#define WASM_VERSION    1

struct wasm_hdr {
    uint32_t    magic;
    uint32_t    version;
} __packed;

typedef struct wasm_hdr wasm_hdr;