#include "contracts/hmi_firmware.h"
#include "product/demo_storage.h"
#include "ui/demo_i18n.h"
static demo_domain_store_t domain, presentation, ui;
bool hmi_firmware_compose(hmi_firmware_composition_t *out)
{
    if (!out)
        return false;
    *out = (hmi_firmware_composition_t){.domain = demo_domain_bind(&domain),
                                          .presentation = demo_domain_bind(&presentation),
                                          .ui = demo_domain_bind(&ui),
                                          .locale_init = demo_i18n_init};
    return true;
}
