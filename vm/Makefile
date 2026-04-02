REPO := $(realpath $(CURDIR)/..)
VM_DIR := $(CURDIR)/.vm
SSH_KEY := ~/.ssh/id_ed25519
SSH := ssh -p 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@localhost

IMG_URL := https://cloud-images.ubuntu.com/noble/current/noble-server-cloudimg-arm64.img
IMG := $(VM_DIR)/noble-server-cloudimg-arm64.img

setup:
	mkdir -p $(VM_DIR)/seed
	curl -L -o $(IMG) -C - $(IMG_URL)
	qemu-img create -f qcow2 \
		-b $(IMG) -F qcow2 \
		$(VM_DIR)/disk.qcow2 10G
	cp meta-data.yaml $(VM_DIR)/seed/meta-data
	yq '.ssh_authorized_keys[0] = "'"$$(cat $(SSH_KEY).pub)"'"' cloud-config-template.yaml > $(VM_DIR)/seed/user-data
	mkisofs -output $(VM_DIR)/seed.iso -volid cidata \
		-joliet -rock $(VM_DIR)/seed/user-data $(VM_DIR)/seed/meta-data

start:
	qemu-system-aarch64 \
		-M virt -accel hvf -cpu host \
		-m 2G -smp 2 \
		-display none -serial null -daemonize \
		-pidfile $(VM_DIR)/qemu.pid \
		-drive if=pflash,format=raw,file=/opt/homebrew/share/qemu/edk2-aarch64-code.fd,readonly=on \
		-drive file=$(VM_DIR)/disk.qcow2,if=virtio \
		-drive file=$(VM_DIR)/seed.iso,if=virtio,format=raw \
		-net nic -net user,hostfwd=tcp::2222-:22 \
		-virtfs local,path=$(REPO),mount_tag=host,security_model=mapped-xattr \
		-qmp unix:$(VM_DIR)/qemu.sock,server,nowait

status:
	@echo '{"execute":"qmp_capabilities"}{"execute":"query-status"}' \
		| socat - UNIX-CONNECT:$(VM_DIR)/qemu.sock 2>/dev/null \
		| jq -s '.[] | select(.return.status) | .return.status' \
		|| echo "VM not running"

stop:
	$(SSH) poweroff || true

kill:
	@kill $$(cat $(VM_DIR)/qemu.pid) 2>/dev/null && echo "QEMU killed" || echo "QEMU not running"

provision:
	$(SSH) "apt-get update && apt-get install -y build-essential linux-headers-$$(uname -r) rsync"
	$(SSH) "modprobe 9p && modprobe 9pnet_virtio"
	$(SSH) "mkdir -p /mnt/host"
	$(SSH) "grep -q host /etc/fstab || echo 'host /mnt/host 9p trans=virtio,rw 0 0' >> /etc/fstab"
	$(SSH) "mount /mnt/host 2>/dev/null || true"

sync-headers:
	mkdir -p ../.kheaders
	$(SSH) "rsync -azL --delete root@localhost:2222:/lib/modules/$$(uname -r)/build/include/ /mnt/host/.kheaders/include/"
	$(SSH) "rsync -azL --delete root@localhost:2222:/lib/modules/$$(uname -r)/build/arch/ /mnt/host/.kheaders/arch/"
