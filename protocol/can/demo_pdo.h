#ifndef DEMO_PDO_H
#define DEMO_PDO_H
#include "contracts/hmi_periodic.h"
#define DEMO_PDO_MOTION_ID 0x381u
#define DEMO_PDO_STATUS_ID 0x481u
/**
 * @brief 按当前快照编码公开合成周期数据，不实现 CANopen 状态机或客户协议。
 *
 * 一帧的全部信号都 VALID 且在量程内才算新鲜；不新鲜时数值归零、fresh 位清零，帧照常发出。
 * 每个标识各有一个滚动计数，每编码一次加一。
 */
bool demo_pdo_encode(const hmi_snapshot_t *snapshot, uint32_t now_ms, hmi_can_frame_t *frame);
#endif
