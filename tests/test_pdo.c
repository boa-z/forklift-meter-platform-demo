#include "core/hmi_core.h"
#include "generated/demo_catalog.h"
#include "product/demo_storage.h"
#include "product/product.h"
#include "protocol/can/demo_pdo.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void set_value(hmi_snapshot_t *snapshot, hmi_signal_id_t id, float value, hmi_value_state_t state)
{
    size_t index = hmi_catalog_index(snapshot->catalog, id);
    assert(index < snapshot->catalog->signal_count);
    snapshot->signals[index] = (hmi_value_t){value, 1000u, state, DEMO_SOURCE_CAN};
}

static void emit_vector(const char *name, const hmi_can_frame_t *frame)
{
    printf("%s ", name);
    for (unsigned i = 0u; i < frame->size; ++i)
        printf("%02x", frame->data[i]);
    printf("\n");
}

/* 每次编码推进该标识的滚动计数；先空转到想要的序号再取真正的输出。 */
static hmi_can_frame_t encode_at(const hmi_periodic_frame_t *definition, const hmi_snapshot_t *snapshot,
                                   unsigned skip)
{
    hmi_can_frame_t frame = definition->frame;
    for (unsigned i = 0u; i < skip; ++i)
    {
        frame = definition->frame;
        assert(definition->encode(snapshot, 0u, &frame));
    }
    frame = definition->frame;
    assert(definition->encode(snapshot, 0u, &frame));
    assert(frame.size == 8u);
    return frame;
}

int main(void)
{
    const hmi_product_t *product = hmi_product_get();
    assert(product->periodic_count == 4u);
    demo_domain_store_t storage;
    hmi_core_storage_t binding = demo_domain_bind(&storage);
    hmi_core_t core;
    assert(hmi_core_init(&core, product->catalog, &binding));
    hmi_snapshot_t *domain = &core.snapshot;
    const hmi_signal_id_t ids[] = {METER_SPEED, METER_SOC,     METER_HEIGHT,   METER_LOAD,   METER_SEAT,
                                     METER_BRAKE, METER_NEUTRAL, METER_CHARGING, METER_WARNING};
    const float nominal[] = {12.5f, 75.0f, 1.25f, 780.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f};
    for (unsigned i = 0u; i < sizeof(ids) / sizeof(ids[0]); ++i)
        set_value(domain, ids[i], nominal[i], HMI_VALUE_VALID);
    domain->brightness = 31u;
    domain->revision = 0x1234u;
    domain->generation = 0x12345678u;

    /* 黄金向量：DBC 交叉校验（test_pdo_dbc.py）读取这些输出。 */
    const hmi_periodic_frame_t *motion = &product->periodic[2], *status = &product->periodic[3];
    hmi_can_frame_t frame = encode_at(motion, domain, 127u);
    const uint8_t motion_bytes[] = {0xe2, 4, 0xe2, 4, 0x0c, 3, 75, 255};
    assert(memcmp(frame.data, motion_bytes, 8u) == 0);
    emit_vector("motion", &frame);
    frame = encode_at(status, domain, 255u);
    const uint8_t status_bytes[] = {21, 1, 255, 1, 0x78, 0x56, 0x34, 0x12};
    assert(memcmp(frame.data, status_bytes, 8u) == 0);
    emit_vector("status", &frame);

    /* 两条状态帧：数值、fresh、计数、revision、反码与异或校验。 */
    for (unsigned i = 0u; i < 2u; ++i)
    {
        frame = encode_at(&product->periodic[i], domain, 0u);
        const uint16_t value = i == 0u ? 31u : 1250u;
        assert(frame.data[0] == (uint8_t)value && frame.data[1] == (uint8_t)(value >> 8) && frame.data[2] == 1u);
        assert(frame.data[4] == 0x34u && frame.data[5] == 0x12u && frame.data[6] == (uint8_t)~frame.data[0]);
        uint8_t check = 0u;
        for (unsigned b = 0u; b < 7u; ++b)
            check ^= frame.data[b];
        assert(frame.data[7] == check);
    }
    assert(product->periodic[0].frame.id == 0x3c0u && product->periodic[0].period_ms == 50u &&
           product->periodic[0].critical);
    assert(product->periodic[1].frame.id == 0x2f0u && product->periodic[1].period_ms == 100u &&
           !product->periodic[1].critical);
    assert(!motion->critical && !status->critical);

    /* 非法入口不产生输出。 */
    frame = motion->frame;
    frame.id = 0x123u;
    hmi_can_frame_t before = frame;
    assert(!demo_pdo_encode(domain, 0u, &frame) && memcmp(&frame, &before, sizeof(frame)) == 0);
    assert(!demo_pdo_encode(NULL, 0u, &frame) && !demo_pdo_encode(domain, 0u, NULL));

    /* 任一信号不可用、越界或非有限时整组不新鲜：数值归零、fresh 位清零。 */
    const float invalid[] = {-1.0f, 101.0f, NAN, INFINITY};
    for (unsigned i = 0u; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        set_value(domain, METER_SOC, invalid[i], HMI_VALUE_VALID);
        frame = encode_at(motion, domain, 0u);
        assert(frame.data[0] == 0u && (frame.data[7] & 1u) == 0u);
    }
    const hmi_value_state_t unavailable[] = {HMI_VALUE_UNKNOWN, HMI_VALUE_STALE, HMI_VALUE_ERROR};
    for (unsigned i = 0u; i < sizeof(unavailable) / sizeof(unavailable[0]); ++i)
    {
        set_value(domain, METER_SOC, 75.0f, unavailable[i]);
        frame = encode_at(motion, domain, 0u);
        assert((frame.data[7] & 1u) == 0u);
    }
    frame = encode_at(motion, domain, 0u);
    assert(frame.data[0] == 0u && frame.data[6] == 0u);
    emit_vector("stale_motion", &frame);
    set_value(domain, METER_SEAT, 0.5f, HMI_VALUE_VALID);
    frame = encode_at(status, domain, 0u);
    assert(frame.data[0] == 0u && frame.data[1] == 0u);
    emit_vector("stale_status", &frame);

    /* 零是合法的新鲜值。 */
    for (unsigned i = 0u; i < 4u; ++i)
        set_value(domain, ids[i], 0.0f, HMI_VALUE_VALID);
    frame = encode_at(motion, domain, 0u);
    assert(frame.data[0] == 0u && (frame.data[7] & 1u) == 1u);
    emit_vector("zero_motion", &frame);
    return 0;
}
