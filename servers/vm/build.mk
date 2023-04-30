objs-y += main.o task.o bootfs.o pm.o page_fault.o bootfs_image.o
cflags-y += -DBOOTFS_PATH='"$(bootfs_bin)"' -DBOOT_SERVERS='"$(BOOT_SERVERS)"'

$(build_dir)/bootfs_image.o: $(bootfs_bin)
