# High Complexity: Execution Speed Optimizations

These are fundamental architectural changes that touch nearly every component. Each provides order-of-magnitude improvements in specific areas but requires careful, staged implementation.

---

## 1. Compact Bytecode Encoding + Computed-Goto Dispatch

**Files to create/modify:**
- `src/bytecode/src/bytecode.cppm` — rewrite `Instruction` variant as flat opcodes + operands
- `src/bytecode/src/bytecode.cpp` — rewrite serialization/formatting
- `src/bytecode/CMakeLists.txt` — possibly split the encoding module
- `src/compiler/src/compiler.cpp` — all `emit()` calls change to new format
- `src/compiler/src/compiler.cppm` — `Chunk` class changes
- `src/vm/src/vm.cpp` — rewrite `execute_loop()` with computed goto
- `src/vm/src/vm.cppm` — remove old `execute_op` overloads
- `src/bytecode/src/formatting.cpp` (if exists) — update formatter
- `test/snapshot/expected/*/bytecode.txt` — update all expected outputs

**The Problem (detailed):**

**a) Instruction size overhead**
Each instruction is a `std::variant<OpReturn, OpConstant, OpAdd, ..., OpForLoop>` with 29 alternatives. The variant carries an 8-byte discriminant. Each instruction struct uses `std::size_t` (8 bytes) for every field. Average instruction size: **~24-40 bytes**.

Compare to Lua 5.4: 4 bytes per instruction (32-bit opcode + operands packed into 32 bits). LuaJIT: 8 bytes per instruction (64-bit with 2 operands). The Lang3 representation is 3-10x larger per instruction, causing:
- Poor I-cache utilization: fewer instructions fit in L1 cache.
- More memory bandwidth for the bytecode stream.
- More TLB pressure.

**b) Dispatch overhead**
`std::visit` on a 29-alternative variant generates:
1. Read the discriminant index from the variant.
2. Compare against the index table (binary search or switch).
3. Call through a function pointer or generic lambda.
4. The `[[clang::noinline]]` on every `execute_op` forces an additional call.

A computed-goto dispatch (using GCC/Clang `&&` label pointers) generates:
1. Read opcode byte from the bytecode stream.
2. Index into a static array of label addresses.
3. Jump directly to the handler.

**The Fix:**

### Stage 1: Flatten to opcode byte + operand bytes

Define a compact instruction format:

```cpp
// Compile-time descriptor for each opcode
enum class Opcode : uint8_t {
    Return,
    Constant,
    Pop,
    Duplicate,
    Add, Sub, Mul, Div, Mod, Negate,
    Equal, NotEqual, Greater, GreaterEqual, Less, LessEqual,
    Not,
    GetGlobal, SetGlobal,
    GetLocal, SetLocal,
    GetUpvalue, SetUpvalue,
    Closure,
    Jump, JumpIf,
    ForLoop,
    Call,
    MakeArray, GetIndex, SetIndex,
    
    Count  // sentinel — must be last
};

// Use variable-length encoding for operands:
// - U8 for small indices (most locals, most constants)
// - U16 for medium indices
// - U32 for large indices (jump offsets, large arrays)
```

Choose a fixed instruction width or variable-width encoding:

**Option A — Fixed 16-byte instruction:**
```
[opcode: 1 byte] [operand0: 8 bytes? or 4?] [operand1: 4 bytes] [padding: 3 bytes]
```
Simple, but still wasteful for ops with no operands (Return, Add, etc.).

**Option B — Variable-width with prefix (like UTF-8):**
```
Small:  [opcode: 1 byte]                — no operands
Medium: [opcode: 1 byte] [u32: 4 bytes] — one operand
Large:  [opcode: 1 byte] [u32: 4 bytes] [u32: 4 bytes]  — two operands
XLarge: [opcode: 1 byte] [u32: 4 bytes] [u32: 4 bytes] [u32: 4 bytes]
```
Operands are `uint32_t` instead of `size_t`. On 64-bit, 32 bits is sufficient for any reasonable bytecode program (max 4 billion instructions, constants, locals).

**Option C — Fixed 8-byte instruction (recommended, Lua-like):**
```
[opcode: 1 byte] [A: 3 bytes] [B: 4 bytes]
or
[opcode: 1 byte] [Ax: 7 bytes]
```
Each opcode defines how to interpret the operand bytes. Most operations map to:
- `OpConstant{x}`: `[CONSTANT(0)] [x: 24-bit unsigned]` — supports 16M constants
- `OpGetLocal{x}`: `[GET_LOCAL] [x: 24-bit]` — supports 16M locals
- `OpAdd`: `[ADD] [unused: 7 bytes]`
- `OpJump{offset}`: `[JUMP] [offset: 24-bit signed]` — supports ±8M instruction jumps
- `OpCall{n}`: `[CALL] [n: 24-bit]` — supports 16M arguments
- `OpClosure{fn_idx, upvalues...}`: This is special because it carries a variable-length upvalue list. Use a prefix instruction:
  - `CLOSURE_PREP{fn_idx}`: `[CLOSURE_PREP] [fn_idx: 24-bit]` + following `UPVALUE{kind, idx}` instructions for each upvalue.

### Stage 2: Rewrite the compiler to emit compact bytecode

The `Chunk` class changes:

```cpp
// Before:
class Chunk {
    std::vector<Instruction> code;  // vector of variants, ~24-40 bytes each
    std::vector<location::Location> locations;
};

// After:
class Chunk {
    std::vector<uint8_t> code;      // flat bytecode stream
    std::vector<location::Location> locations;  // 1:1 with instructions (sparse map)
};
```

Compilation changes from:
```cpp
void emit(OpConstant op) { code.push_back(Instruction{op}); }
```
To:
```cpp
void emit_constant(uint32_t index) {
    code.push_back(static_cast<uint8_t>(Opcode::Constant));
    append_uint24(code, index);
}
```

The `OpClosure` is the trickiest because it has a variable-length upvalue list:

```cpp
void emit_closure(uint32_t fn_idx, const std::vector<Upvalue> &upvalues) {
    code.push_back(static_cast<uint8_t>(Opcode::Closure));
    append_uint24(code, fn_idx);
    append_uint8(code, static_cast<uint8_t>(upvalues.size()));
    for (auto &uv : upvalues) {
        append_uint8(code, static_cast<uint8_t>(uv.kind));   // local or upvalue
        append_uint24(code, uv.index);
    }
}
```

### Stage 3: Computed-goto dispatch

The VM execution loop changes from:

```cpp
// Current (simplified):
void BytecodeVM::execute_loop(std::size_t target_frames) {
    while (frames.size() > target_frames) {
        maybe_gc();
        auto &frame = frames.back();
        auto &chunk = chunks[frame.chunk_id];
        auto &inst = chunk.code[frame.ip++];
        match::match(inst, [&](const auto &op) {
            execute_op(op, frame);
        });
    }
}
```

To:

```cpp
// With computed goto:
void BytecodeVM::execute_loop(std::size_t target_frames) {
    static void *dispatch_table[] = {
        &&OP_RETURN, &&OP_CONSTANT, &&OP_POP, &&OP_DUPLICATE,
        &&OP_ADD, &&OP_SUB, &&OP_MUL, &&OP_DIV, &&OP_MOD, &&OP_NEGATE,
        &&OP_EQUAL, &&OP_NOT_EQUAL, &&OP_GREATER, &&OP_GREATER_EQUAL,
        &&OP_LESS, &&OP_LESS_EQUAL, &&OP_NOT,
        &&OP_GET_GLOBAL, &&OP_SET_GLOBAL,
        &&OP_GET_LOCAL, &&OP_SET_LOCAL,
        &&OP_GET_UPVALUE, &&OP_SET_UPVALUE,
        &&OP_CLOSURE,
        &&OP_JUMP, &&OP_JUMP_IF,
        &&OP_FOR_LOOP,
        &&OP_CALL,
        &&OP_MAKE_ARRAY, &&OP_GET_INDEX, &&OP_SET_INDEX,
    };
    static_assert(std::size(dispatch_table) == static_cast<size_t>(Opcode::Count));
    
    while (frames.size() > target_frames) {
        maybe_gc();
        auto &frame = frames.back();
        auto &chunk = chunks[frame.chunk_id];
        auto *ip = &chunk.code[frame.ip];
        
        goto *dispatch_table[*ip++];
        
    OP_RETURN: {
        // ... handler ...
        goto *dispatch_table[*ip++];
    }
    OP_CONSTANT: {
        uint32_t idx = read_uint24(ip); ip += 3;
        stack.push_back(program.constants[idx].get_value());
        goto *dispatch_table[*ip++];
    }
    OP_GET_LOCAL: {
        uint32_t idx = read_uint24(ip); ip += 3;
        stack.push_back(stack[frame.frame_pointer + idx]);
        goto *dispatch_table[*ip++];
    }
    // ... etc ...
    }
}
```

**Note about `std::size_t` indices in the compiler:**
The constant pool index, local slot index, etc. currently use `std::size_t`. With the compact format, they should be capped at 24-bit (16 million). If this is insufficient, use a 32-bit operand format for the affected opcodes.

**Operand encoding helpers:**
```cpp
// Write
void append_uint24(std::vector<uint8_t> &code, uint32_t value) {
    code.push_back((value >> 16) & 0xFF);
    code.push_back((value >> 8) & 0xFF);
    code.push_back(value & 0xFF);
}

// Read
uint32_t read_uint24(const uint8_t *&ip) {
    uint32_t value = (static_cast<uint32_t>(ip[0]) << 16)
                   | (static_cast<uint32_t>(ip[1]) << 8)
                   | static_cast<uint32_t>(ip[2]);
    ip += 3;
    return value;
}
```

### Stage 4: Jump patching

The compiler uses `patch_jump()` to fill in jump offsets after the target is known. With the compact format:

```cpp
// Current:
void patch_jump(std::size_t instruction_index) {
    auto &inst = std::get<OpJump>(current_chunk().code[instruction_index]);
    inst.offset = current_chunk().code.size() - instruction_index - 1;
}

// After:
void patch_jump(std::size_t ip_offset) {
    // ip_offset is the byte offset in the code vector
    std::size_t target = current_chunk().code.size();
    std::size_t jump_offset = target - ip_offset - 4; // opcode(1) + offset(3)
    // Write the offset at ip_offset + 1 (after the opcode byte)
    current_chunk().code[ip_offset + 1] = (jump_offset >> 16) & 0xFF;
    current_chunk().code[ip_offset + 2] = (jump_offset >> 8) & 0xFF;
    current_chunk().code[ip_offset + 3] = jump_offset & 0xFF;
}
```

### Stage 5: Bytecode formatting (for snapshot tests)

The formatter changes from:
```
CONSTANT      0 (nil)
GET_LOCAL     0
ADD
```
To match the new encoding. Keep the same format strings for snapshot compatibility.

### Migration strategy:

1. **Add new encoding alongside the old one**: Keep both `std::vector<Instruction>` and `std::vector<uint8_t>` in `Chunk`. The compiler emits into both simultaneously. The VM uses the new encoding. The formatter reads from either (prefer new).

2. **Remove old encoding**: Once the new encoding is verified, remove `std::vector<Instruction>` and all the old `Instruction` variant types.

3. **Update snapshots**: After the encoding is final, regenerate all expected bytecode snapshots.

### Expected impact:
- **Instruction size**: ~24-40 bytes → ~4 bytes per instruction average (6-10x reduction)
- **Dispatch overhead**: `std::visit` + function call → single computed goto (est. 2-5x faster dispatch)
- **Combined throughput improvement**: 2-3x on tight loops (based on similar transitions in Lua, Python, and Ruby interpreters)
- **Trade-off**: 24-bit operand limit (16M) may need to be raised if programs grow large. Use 32-bit operands for safety if the 3-byte savings is not critical.

---

## 2. Value Representation: NaN-Boxing / Tagged Pointers

**Files to create/modify:**
- `src/runtime/src/value.cppm` — rewrite `Value` class entirely
- `src/runtime/src/value.cpp` — rewrite all operations
- `src/runtime/src/primitive.cppm` — potentially absorbed into Value
- `src/runtime/src/gc_value.cppm` — may change how GCValue stores values
- `src/vm/src/vm.cpp` — adjust for new value representation
- All files that construct or inspect `Value` objects

### Background: What is NaN-boxing?

On x86-64, IEEE 754 `double` has 64 bits:
- 1 sign bit
- 11 exponent bits
- 52 mantissa bits (including implicit leading 1 for normal numbers)

NaN is represented by exponent = 0x7FF (all 1s) with a non-zero mantissa. The mantissa has 52 bits, but only the most significant bit distinguishes quiet vs. signaling NaN. This leaves **51 bits** of payload in a NaN representation that can be used to store arbitrary data.

NaN-boxing exploits this by:
- Non-NaN `double` values: stored as-is (the VM's float type).
- All other types: stored as NaN with the type tag and data in the mantissa bits.

On x86-64, user-space pointers have only 48 significant bits. The upper 16 bits (bits 48-63) must be copies of bit 47 (sign extension) for canonical addresses. This means those 16 high bits can be used to smuggle a type tag **if** the pointer is stored in a NaN payload.

### Design for Lang3:

A 64-bit NaN-boxed value layout:

```
Bits:  [63] [62:52]     [51:48]       [47:0]
       sign  exponent    type tag      payload
```

- **Type tag (4 bits)**: Up to 16 types (Nil, Bool, Int, Float, String, Vector, Function, etc.)
- **Payload (48 bits)**: For pointers (string, vector, function), this is the heap address. For immediates (nil, bool, small int), this is the value itself.

**For immediate types (no heap allocation):**
- `Nil`: tag = NIL, payload = 0
- `Bool`: tag = BOOL, payload = 0 or 1
- `Int` (small): tag = INT, payload = 48-bit signed integer
- `Float`: stored as a normal IEEE 754 double (all bits used for the float value)

**For heap types (string, vector, function):**
- The payload is a 48-bit pointer to a GC-managed `GCValue`.
- The NaN-boxing uses the NaN representation: exponent = 0x7FF, mantissa bits encode (type_tag << 48) | pointer.

```cpp
// Value becomes a single 64-bit integer
class Value {
    uint64_t bits;
    
    enum Tag : uint64_t {
        TAG_NIL     = 0,
        TAG_BOOL    = 1,
        TAG_INT     = 2,
        TAG_STRING  = 3,
        TAG_VECTOR  = 4,
        TAG_FUNCTION = 5,
        // ... more types
        
        TAG_SHIFT   = 48,  // bits 48-51 hold the tag
        PAYLOAD_MASK = (1ULL << 48) - 1,
    };
    
    // Mask for NaN: exponent bits all 1
    static constexpr uint64_t NAN_MASK = 0x7FF0000000000000ULL;
    static constexpr uint64_t TAG_MASK = 0x000F000000000000ULL;  // 4-bit tag at bits 48-51
    
    // For non-NaN doubles, the entire 64-bit value is the double
    // For tagged types, the value is a NaN with tag in bits 48-51
};
```

**For `double`:**
```cpp
static Value from_double(double d) {
    return Value{std::bit_cast<uint64_t>(d)};
}

double as_double() const {
    // The raw bits represent a valid double (no NaN payload masking needed)
    return std::bit_cast<double>(bits);
}

bool is_double() const {
    // Check if exponent is NOT all 1s (not NaN)
    return (bits & NAN_MASK) != NAN_MASK;
}
```

**For tagged types:**
```cpp
static Value nil() {
    return Value{NAN_MASK | (TAG_NIL << TAG_SHIFT)};
}

static Value from_bool(bool b) {
    return Value{NAN_MASK | (TAG_BOOL << TAG_SHIFT) | (b ? 1 : 0)};
}

static Value from_int(int64_t i) {
    // i must fit in 48 bits
    return Value{NAN_MASK | (TAG_INT << TAG_SHIFT) | (static_cast<uint64_t>(i) & PAYLOAD_MASK)};
}

static Value from_gc_ptr(Tag tag, GCValue *ptr) {
    return Value{NAN_MASK | (tag << TAG_SHIFT) | reinterpret_cast<uint64_t>(ptr)};
}
```

**Type checks are fast:**
```cpp
Tag get_tag() const {
    return static_cast<Tag>((bits >> TAG_SHIFT) & 0xF);
}

bool is_nil() const { return get_tag() == TAG_NIL; }
bool is_bool() const { return get_tag() == TAG_BOOL; }
bool is_int() const { return get_tag() == TAG_INT; }
bool is_double() const { return (bits & NAN_MASK) != NAN_MASK; }
bool is_string() const { return get_tag() == TAG_STRING; }
// etc.
```

### Impact on the GC:

The GC currently stores `GCValue` objects with a `Value` member. With NaN-boxing, a `GCValue` pointer is just a 48-bit address. The GC mark phase still needs to trace references.

The key change: there is no longer a `shared_ptr<HeapValue>`. Heap-allocated objects (strings, vectors, functions) are pointed to directly via the NaN-boxed pointer. The `Value` is just 8 bytes (was 24). The `HeapValue` class can still exist, but it's pointed to via a raw pointer embedded in the NaN-boxed `Value`.

### Challenges:

1. **48-bit address limitation**: If the heap uses more than 48 bits of address space (possible with ASLR on some systems), the pointer won't fit. On x86-64, current hardware uses 48-bit (256 TB) or 57-bit (with 5-level paging). For 5-level paging, pointers have 57 significant bits, leaving only 7 bits for the tag (not enough for NaN-boxing in the standard form).

   **Mitigation**: Use `mmap` with `MAP_FIXED` to allocate GC memory in the lower 48-bit address range. Or use `SETARCH` / `ADDR_NO_RANDOMIZE`. Or accept that 5-level paging systems may fall back to a slower path.

2. **48-bit signed integers only**: The int type is limited to 47 bits of magnitude (sign bit + 47 data bits). For 64-bit integers, fall back to a heap-allocated "big int" type.

3. **Alignment requirement**: `GCValue` pointers must have the lower bits clear (aligned to 8+ bytes). The 3 low bits of a pointer are always 0, so they can encode additional information or just be ignored.

4. **GC scanning**: The mark phase must scan NaN-boxed values on the stack and in frames to find GC pointers. Currently it knows the type of each `Value` via the variant. With NaN-boxing, it must:
   - Check if the value is a heap type (string, vector, function).
   - Extract the 48-bit pointer.
   - Mark the pointed-to `GCValue`.
   - For vectors: scan their `Ref` elements (which are also NaN-boxed values now).

5. **`Ref` type becomes `Value`**: Currently `Ref` wraps `reference_wrapper<GCValue>`. With NaN-boxing, a `Ref` can simply be a `Value` containing a pointer to the `GCValue` (since NaN-boxed values can hold direct pointers). But `Ref` must continue to be non-nullable and point to GC memory. An alternative: make `Ref` a thin wrapper over `Value` that only holds GC-pointer types.

### Performance expectations:

NaN-boxing eliminates:
- The 24-byte `Value` → 8-byte `Value` (3x less stack memory).
- The `shared_ptr` reference counting (no atomic ops on value copy).
- The double `std::visit` dispatch.
- All heap allocation for booleans, nil, and small integers.

Operations become extremely cheap:
- Type check: 1 bitwise AND + compare.
- Extract pointer: 1 bitwise AND.
- Compare two values (for equality): just 1 `uint64_t` compare in most cases (if both are the same type, no pointer dereference needed).

### Implementation stages:

1. **Design the bit layout**: Choose exact bit positions for tags, payload, and NaN masking.
2. **Implement `Value` class**: Write the new 64-bit `Value` with all accessors and constructors.
3. **Rewrite `GCValue::mark()`**: Scanning must extract pointers from NaN-boxed values.
4. **Implement `Value::visit()`**: The core dispatch — now a simple switch on the 4-bit tag instead of nested `std::visit`s.
5. **Handle 48-bit pointers in GC**: Ensure all GC allocations happen in the lower 48-bit address space.
6. **Unit tests**: Test all type conversions, edge cases (NaN boxing for actual NaN doubles vs tagged NaNs).
7. **Integration tests**: Run all snapshot tests.

---

## 3. String Interning

**Files to create/modify:**
- `src/runtime/src/string_table.cppm` — new module for interned string table
- `src/runtime/src/string_table.cpp` — implementation
- `src/runtime/CMakeLists.txt` — add new module
- `src/runtime/src/value.cppm` — change string type to interned ID
- `src/runtime/src/value.cpp` — update `add`, `compare`, `index`, etc.
- `src/runtime/src/gc_value.cppm` — update string handling
- `src/vm/src/builtins.cpp` — update string-related builtins

### The Problem:

Every string runtime operation creates a new `std::string`:
- String concatenation (`"a" + "b"`): creates `"ab"`
- Substring indexing: creates a new string
- Builtins like `str()` or `input()`: create new strings
- Global name resolution: heap-allocates string keys for `std::unordered_map`

String comparison is O(n) character-by-character. With interning, comparison becomes O(1) pointer comparison.

### The Fix:

Add a global string table (per-program or per-VM) that interns all strings:

```cpp
class StringTable {
    // Owns a chunk of memory for interned strings
    std::unordered_map<std::string_view, InternedString> table;
    // String data stored in contiguous blocks (arena allocation)
    std::vector<std::unique_ptr<char[]>> arenas;
    char *current_arena;
    size_t arena_remaining;
};
```

An `InternedString` is a lightweight handle:

```cpp
class InternedString {
    const char *data;      // pointer into the arena (stable, never freed)
    uint32_t size;         // length (for fast size checks)
    uint64_t hash;         // precomputed hash
    
    bool operator==(const InternedString &other) const {
        return data == other.data;  // O(1) comparison!
    }
};
```

**Integration with `Value`:**

Currently `Value`'s heap type includes `std::string` directly. With interning:

```cpp
// Before:
class HeapValue {
    std::variant<std::unique_ptr<Function>, std::vector<Ref>, std::string> inner;
};

// After (with NaN-boxing from §2, all types are in the tag):
class Value {
    // For strings: tag = STRING, payload = pointer to InternedString (in string table arena)
    InternedString *as_interned() const { ... }
};
```

If NaN-boxing is not yet done, an intermediate form:

```cpp
// Value variant changes from std::string to InternedString:
class Value {
    std::variant<Nil, Primitive, InternedString, std::vector<Ref>, Function> inner;
};
```

**String operations with interning:**

- **Concatenation**: Must allocate a new interned string (the concatenated result is a new entry in the table).
- **Substring**: May or may not be interned. For short substrings (fits in SSO), intern. For long substrings from a larger string, consider a `std::string_view`-style slice (requires tracking the parent string's lifetime).
- **Comparison**: Becomes O(1) `data == other.data` for equality, but sorting order requires the actual character data (can fall back to `memcmp`).
- **Hashing**: Precomputed hash makes `std::unordered_map` insertion/lookup cheaper.

**Memory management:**

Interned strings are never freed (permanent). Arena allocation is more efficient than per-string `malloc`:
- No per-string allocation overhead.
- Better cache locality (strings from the same arena are adjacent).
- Simpler lifetime management.

**Cross-cutting with GC:**

If interned strings are permanent, the GC doesn't need to mark them. The `StringTable` owns the arena and keeps all strings alive. GC scanning skips string type values (no GC pointer to chase).

**Integration with global variables:**

The VM's `global_symbols` is `std::unordered_map<std::string, runtime::Ref>`. With interning, the key can be an `InternedString`:

```cpp
std::unordered_map<InternedString, runtime::Ref> global_symbols;
```

This reduces rehashing and string duplication on global variable access.

**Performance expectations:**

- **Equality**: O(n) → O(1) for string comparison.
- **Memory**: Less fragmentation, fewer allocations. Arena allocation is faster than per-string `new`.
- **Global variable access**: Faster hash table lookups (better hash quality, no key copies).
- **Concatenation overhead**: Still O(n) for the concatenation itself, but uses the fast arena allocator.

---

## Relationships Between High-Complexity Changes

These three optimizations are deeply interconnected:

```
NaN-boxing (§2) provides:
  └──→ Compact Value (8 bytes instead of 24)
  └──→ Direct pointer storage (no shared_ptr)
  └──→ Required for: efficient string interning handles
  
String Interning (§3) provides:
  └──→ O(1) string comparison
  └──→ Better hash table performance in globals
  └──→ Arena allocation (faster than per-object malloc)
  └──→ Simpler GC (no tracing through strings)
  └──→ Fits naturally into NaN-boxed Value

Compact Bytecode (§1) is independent of value representation:
  └──→ Affects: compiler emission, VM dispatch, bytecode format
  └──→ Does NOT depend on: Value layout, GC, string representation
  └──→ Can be implemented separately, in parallel with §2 and §3
```

**Bug tracking:** All bugs discovered during implementation of these (or any other)
optimizations should be documented in `plan/04_bugs.md` with reproducer, root cause,
and fix. This is especially important for GC-related bugs that may be subtle and
difficult to reproduce.

**Recommended order:**
1. **String interning** first (lowest risk of the three, can be done without changing Value representation fundamentally).
2. **NaN-boxing** second (builds on the string internals, removes `shared_ptr` overhead, compresses Value).
3. **Compact bytecode** independently at any time (touches different files, no overlap with value representation).
