#include "sandstone_p.h"
#include "topology.h"

int num_devices() {
    return sApp->thread_count;
}

void apply_deviceset_param(char *param) {
    // TODO
}

void restrict_topology(DeviceRange range) {
    // TODO
}

void init_topology() {
    // TODO
}

uint64_t retrieve_physical_address(const volatile void *ptr) {
    // not suppotred
    return 0;
}