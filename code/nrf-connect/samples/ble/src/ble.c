#include "ble.h"

#include <prstlib/macros.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "encoding.h"

LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

static uint8_t service_data[CONFIG_PRST_BLE_ENCODING_SERVICE_DATA_LEN] = {0};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_SVC_DATA16, service_data, ARRAY_SIZE(service_data)),
    BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME),
};

static const struct bt_data sd[] = {};

static bt_addr_le_t mac_addr;

static int set_user_defined_bt_addr(const char *addr_str) {
  bt_addr_le_t addr;
  RET_IF_ERR(bt_addr_le_from_str(addr_str, "random", &addr));
  RET_IF_ERR(bt_id_create(&addr, NULL));
  return 0;
}

// bt_addr_le_t.a holds the MAC address in big-endian.
static int get_mac_addr(bt_addr_le_t *out) {
  struct bt_le_oob oob;
  RET_IF_ERR(bt_le_oob_get_local(BT_ID_DEFAULT, &oob));
  const uint8_t *addr = oob.addr.a.val;
  LOG_INF("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x",
          addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
  *out = oob.addr;
  return 0;
}

int prst_ble_init() {
#if CONFIG_PRST_BLE_HAS_USER_DEFINED_RANDOM_STATIC_ADDR
  RET_IF_ERR(set_user_defined_bt_addr(CONFIG_PRST_BLE_USER_DEFINED_RANDOM_STATIC_ADDR));
#else
  UNUSED_OK(set_user_defined_bt_addr);
#endif  // PRST_BLE_HAS_USER_DEFINED_MAC_ADDR

  RET_IF_ERR(bt_enable(/*bt_reader_cb_t=*/NULL));
  if (IS_ENABLED(CONFIG_SETTINGS)) {
    RET_IF_ERR_MSG(settings_load(), "Error in settings_load()");
  }

  RET_IF_ERR(get_mac_addr(&mac_addr));
  return 0;
}

int prst_ble_adv_start() {
  // If BT_LE_ADV_NCONN_IDENTITY, this function will advertise with a static MAC
  // address programmed in the chip. If BT_LE_ADV_NCONN, this function returns
  // advertises with a random MAC each time.
  return bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd,
                         ARRAY_SIZE(sd));
}

int prst_ble_adv_stop() {
  return bt_le_adv_stop();
}

int prst_ble_adv_set_data(const prst_sensors_t *sensors) {
  return prst_ble_encode_service_data(sensors, &mac_addr, service_data,
                                      sizeof(service_data));
}