.PHONY: all clean install help

all: hello.bin hello64.bin echo.bin

hello.bin: hello.asm
	nasm -f bin hello.asm -o hello.bin

hello64.bin: hello64.asm
	nasm -f bin hello64.asm -o hello64.bin

echo.bin: echo.asm
	nasm -f bin echo.asm -o echo.bin

clean:
	rm -f *.bin

help:
	@echo "Example Programs Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build all example programs"
	@echo "  make install  - Copy binaries to disk image (requires sudo/mtools)"
	@echo "  make manual   - Show manual installation steps"
	@echo "  make clean    - Remove compiled binaries"
	@echo ""

manual:
	@echo "Manual installation steps:"
	@echo ""
	@echo "Option 1: Using mtools (if installed):"
	@echo "  mcopy -i ../../build/disk.img -o *.bin ::"
	@echo ""
	@echo "Option 2: Mount and copy:"
	@echo "  sudo mkdir -p /mnt/osdisk"
	@echo "  sudo mount -o loop ../../build/disk.img /mnt/osdisk"
	@echo "  sudo cp *.bin /mnt/osdisk/"
	@echo "  sudo umount /mnt/osdisk"
	@echo ""
	@echo "Option 3: Use the OS write command:"
	@echo "  1. Run 'make run' in parent directory"
	@echo "  2. In the OS shell, create files:"
	@echo "     touch hello.bin"
	@echo "  3. Use 'v hello.bin' to open the editor"
	@echo "  4. Paste the binary content (not practical for binaries)"
	@echo ""
	@echo "Recommended: Use Option 1 (mtools) or Option 2 (mount)"
	@echo ""

install: all
	@if [ ! -f ../../build/disk.img ]; then \
		echo "Error: ../../build/disk.img does not exist."; \
		echo ""; \
		echo "First, create the disk image by running:"; \
		echo "  cd ../.. && make run"; \
		echo ""; \
		echo "Then try 'make install' again."; \
		exit 1; \
	fi
	@echo "Installing binaries to disk image..."
	@echo ""
	@if command -v mcopy >/dev/null 2>&1; then \
		echo "Using mtools (mcopy)..."; \
		mcopy -i ../../build/disk.img -o *.bin :: && \
		echo "Success! Binaries installed." && \
		echo "Run 'cd ../.. && make run' and use 'exec <program>' in the OS."; \
	else \
		echo "mtools not found. Trying mount method..."; \
		mkdir -p /tmp/osdisk && \
		sudo mount -o loop ../../build/disk.img /tmp/osdisk && \
		sudo cp *.bin /tmp/osdisk/ && \
		sudo umount /tmp/osdisk && \
		rmdir /tmp/osdisk && \
		echo "Success! Binaries installed." && \
		echo "Run 'cd ../.. && make run' and use 'exec <program>' in the OS." || \
		(rmdir /tmp/osdisk 2>/dev/null; \
		 echo ""; \
		 echo "Installation failed. Try:"; \
		 echo "  sudo apt install mtools"; \
		 echo "Then run 'make install' again."; \
		 exit 1); \
	fi
