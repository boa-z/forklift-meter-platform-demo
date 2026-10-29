#include "product/product.h"
static const hmi_frame_route_t entries[] = {
#include "generated/can/routes.inc"
};
const hmi_route_profile_t meter_demo_routes = {entries, sizeof(entries) / sizeof(entries[0])};
