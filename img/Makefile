# Use ditaa to generate figures

DITAA=ditaa

all: secure-boot-flow_esp-idf secure-boot-flow_esp32-mcuboot

secure-boot-flow_esp-idf: secure-boot-flow_esp-idf.txt
	$(DITAA) -oE secure-boot-flow_esp-idf.txt

secure-boot-flow_esp32-mcuboot: secure-boot-flow_esp32-mcuboot.txt
	$(DITAA) -oE secure-boot-flow_esp32-mcuboot.txt