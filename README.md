# BasicMicrokernel
## How to run it on Codespaces:
Firstly, install all the dependencies at ../BasicMicrokernel with the command:

```bash
sudo apt update && sudo apt install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf qemu-system-misc make gdb-multiarch
```

To execute the system, use the command `make beauty`, it sends the log messages to a log `.txt` file, also runs the system and cleans the memory before execution.

## Implemented functionalities

- First Fit Memory
- Allocation and Deallocation of Tasks
- Fragmentation Calculous
- Memory Heap Diagram
- Heap Status
- Coalescence
- Block Reuse
- Block Split
- Memory free




