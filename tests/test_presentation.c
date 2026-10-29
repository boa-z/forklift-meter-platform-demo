#include "application/presentation.h"
#include "core/hmi_core.h"
#include "product/demo_storage.h"
#include <assert.h>
#include <math.h>
int main(void)
{
    demo_domain_store_t storage;
    hmi_core_storage_t binding = demo_domain_bind(&storage);
    hmi_core_t core;
    assert(hmi_core_init(&core, &meter_demo_catalog, &binding));
    demo_presentation_t view;
    demo_presentation_build(&core.snapshot, &view);
    assert(view.speed.state == HMI_VALUE_UNKNOWN && view.link == DEMO_LINK_OFFLINE);
    assert(view.hours.state == HMI_VALUE_UNKNOWN);
    assert(view.mileage.state == HMI_VALUE_VALID && fabsf(view.mileage.value - 1286.4f) < 0.01f);
    /* 小时计必须保留快照有效性，不能把未知、过期或错误伪装成零。 */
    for (unsigned state = HMI_VALUE_UNKNOWN; state <= HMI_VALUE_ERROR; ++state)
    {
        hmi_update_t hours = {METER_WORK_HOURS, {1246, 1, (hmi_value_state_t)state, DEMO_SOURCE_CAN}};
        bool applied = hmi_core_apply(&core, &hours);
        assert(applied);
        demo_presentation_build(&core.snapshot, &view);
        assert(view.hours.value == 1246 && view.hours.state == (hmi_value_state_t)state);
    }
    hmi_update_t hours = {METER_WORK_HOURS, {0, 1, HMI_VALUE_VALID, DEMO_SOURCE_CAN}};
    bool applied = hmi_core_apply(&core, &hours);
    assert(applied);
    demo_presentation_build(&core.snapshot, &view);
    assert(view.hours.value == 0 && view.hours.state == HMI_VALUE_VALID);
    hmi_core_connection(&core, true, 1);
    demo_presentation_build(&core.snapshot, &view);
    assert(view.link == DEMO_LINK_WAITING);
    hmi_update_t update = {METER_SPEED, {10, 1, HMI_VALUE_VALID, DEMO_SOURCE_CAN}};
    assert(hmi_core_apply(&core, &update));
    assert(hmi_core_action(&core, &(hmi_action_t){HMI_ACTION_UNITS, 0, 1}));
    demo_presentation_build(&core.snapshot, &view);
    assert(fabsf(view.speed.value - 6.21371f) < 0.0001f && view.speed.state == HMI_VALUE_VALID);
    assert(view.speed_maximum == 32 && view.link == DEMO_LINK_CONNECTED);
    update.value.state = HMI_VALUE_STALE;
    assert(hmi_core_apply(&core, &update));
    assert(view.speed.state == HMI_VALUE_VALID);
    demo_presentation_build(&core.snapshot, &view);
    assert(view.speed.state == HMI_VALUE_STALE && view.link == DEMO_LINK_STALE);
    update.value.state = HMI_VALUE_ERROR;
    assert(hmi_core_apply(&core, &update));
    demo_presentation_build(&core.snapshot, &view);
    assert(view.speed.state == HMI_VALUE_ERROR);
    hmi_action_t intent = demo_speed_limit_intent(30);
    assert(view.limit != 30 && hmi_core_action(&core, &intent));
    demo_presentation_build(&core.snapshot, &view);
    assert(view.limit_available && view.limit == 30);
    assert(hmi_snapshot_fault_set(&core.snapshot, DEMO_FAULT_LOW_CHARGE, true));
    demo_presentation_build(&core.snapshot, &view);
    assert(view.faults[0].active && view.faults[0].code == DEMO_FAULT_LOW_CHARGE);
    assert(hmi_snapshot_profile(&core.snapshot, true, 1, 4));
    demo_presentation_build(&core.snapshot, &view);
    assert(hmi_profile_matches(&view.profile, core.snapshot.profile.generation));
    assert(hmi_snapshot_profile(&core.snapshot, true, 2, 0));
    assert(!hmi_profile_matches(&core.snapshot.profile, view.profile.generation));
    /* 根据当前快照副本重新构建，防止旧系列配置被当作当前配置。 */
    demo_presentation_build(&core.snapshot, &view);
    assert(view.profile.family == 2 && view.profile.capabilities == 0);
    return 0;
}
