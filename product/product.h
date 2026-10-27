#ifndef DEMO_PRODUCT_H
#define DEMO_PRODUCT_H
#include "contracts/meter_product.h"
extern const meter_protocol_profile_t meter_demo_protocols;
extern const meter_route_profile_t meter_demo_routes;
extern const meter_ui_factory_t meter_demo_ui;
/** @brief 运行时线程每轮的 Demo 规则：发布设置服务状态并按阈值判定告警。 */
void meter_demo_evaluate(meter_snapshot_t *snapshot, uint32_t now_ms);
#endif
