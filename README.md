# wasmos 
<p align="center">
  <img src="https://github.com/r1ru/wasmos/assets/92210252/dc6172ef-d8fe-4920-985d-5b14a22ad79d">
</p>

wasmos is a microkernel developed based on [HinaOS](https://github.com/nuta/microkernel-book).
It implements a WebAssembly (WASM) "userland", and all WASM binaries are executed in Ring 0 using [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime).
See my [blog](https://medium.com/@r1ru/wasmos-a-proof-of-concept-microkernel-that-runs-webassembly-natively-850043cad121) for more details.

![wasmos](https://github.com/RI5255/wasmos/assets/92210252/9bccd926-6260-4d1c-947a-68df5e452d7d)

## Quickstart
First clone this repository, remember to add the --recursive option as wasmos uses [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime) as a submodule.
```
git clone --recursive https://github.com/r1ru/wasmos
```
Next, build and run wasmos. The easiest way is to use [Dev Container](https://code.visualstudio.com/docs/devcontainers/containers). Launch the container and execute following commands.
If you're not using it, please refer to the [Dockerfile](https://github.com/r1ru/wasmos/blob/main/.devcontainer/Dockerfile), install the required packages, and build it.

```
make                # Build wasmos
make V=1            # Build wasmos (Output detailed build logs)
make run            # Run wasmos in qemu (single core)
make run CPUS=4     # Run wasmos in qemu (4 cores)
```
If it starts successfully, the shell server is launched and you can execute following commands.

```
start hello         # Run a helllo-world program
start hello_wasmvm  # Run a program that uses the system call sys_wasmvm, which generates tasks from wasm binaries
start wasm_ping     # Run a wasm binary that uses message passing APIs
start wasm_webapi   # Run a toy web server (wasm binary). Access localhost:1234 to see the page
```

## Contributing
We accept bug reports, feature requests, and patches on [GitHub](https://github.com/r1ru/wasmos).

## Similar Projects
- [Nebulet](https://github.com/nebulet/nebulet)
- [Wasmachine](https://ieeexplore.ieee.org/document/9156135)
- [kernel-wasm](https://github.com/wasmerio/kernel-wasm)
