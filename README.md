# GSDB
**Geometric Storage Database**

GSDB is a high-performance, embedded document database powered by a constant-time geometric engine. It utilizes Space-Filling Curves (SFC) to map 4D coordinates directly to physical disk offsets, enabling **O(1)** lookups regardless of dataset size.

![CLI Session](assets/demo_cli.png)

## Architecture
Traditional databases use B-Trees (O(log N)) or Linear Scans (O(N)). GSDB uses **Geometric Indexing**:
1.  **Input**: Key (e.g., "user_123")
2.  **Hash**: FNV-1a -> 64-bit Hash
3.  **Geometry**: Hash -> 4D Coordinate `(x, y, z, t)`
4.  **access**: Direct file offset calculation.

**Result**: Constant-time access. A lookup takes the same time whether there are 1,000 records or 1,000,000,000.

## Performance
Measured on Apple Silicon (M-Series):

- **Ingestion**: 1,400,000+ ops/sec
- **Lookup Latency**: 1 microsecond (avg)
- **P99 Latency**: 18 microseconds
- **Scalability**: Linear throughput, Constant latency.

## Specifications
- **Language**: ISO C11 (Standard C)
- **Dependencies**: None (Zero external libraries)
- **Network**: Built-in TCP Server (Port 54321)
- **Storage**: Append-only Heap + Sparse Index

## Quick Start

### Build
```bash
make all
```

### Interactive Shell
```bash
./gsdb
```
```javascript
GSDB > db.insert({"key": "sys_core", "status": "online", "latency": 0})
{ "status": "ok", "id": 1, "key": "sys_core" }
(0.0003s)

GSDB > db.find({"key": "sys_core"})
{ "key": "sys_core", "status": "online", "latency": 0 }
(0.0001s)
```

### Network Daemon
```bash
./gs_server
```
