CONTIKI_PROJECT = air_sample

# coap-example-observe-client is outdated but will be ported at a later point
all: $(CONTIKI_PROJECT)
MODULES_REL+=./resources/machine_learning
MODULES_REL+=./resources
MODULES_REL+=./global_variable
LDFLAGS += -lm
# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

CONTIKI=../../../..

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
INC += /home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn
MODULES_REL += /home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn
MODULES_REL+=../cJSON-master
include $(CONTIKI)/Makefile.include