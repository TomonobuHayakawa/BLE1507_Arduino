#include "BLE1507.h"

#define PRINT_DEBUG

/****************************************************************************
 * static Data
 ****************************************************************************/

static struct ble_common_ops_s ble_common_ops;
static struct ble_gatt_peripheral_ops_s ble_gatt_peripheral_ops;
static struct ble_gatt_central_ops_s ble_gatt_central_ops;
static uint16_t ble_conn_handle;
static struct ble_gatt_service_s *g_ble_gatt_service;
static BLE_ATTR_PERM attr_param;
static uint8_t char_data[BLE_MAX_CHAR_SIZE];
static BLE_CHAR_VALUE char_value;
static BLE_CHAR_PROP char_property;
static struct ble_gatt_char_s g_ble_gatt_char;
static int g_ble_bonded_device_num;
static struct ble_cccd_s **g_cccd = NULL;
static bool ble_is_notify_enabled = false;

static uint16_t last_cccd_handle = BLE_GATT_INVALID_ATTRIBUTE_HANDLE;

static struct ble_state_s *s_ble_state = NULL;
static uint8_t  g_read_data[BLE_MAX_GATT_DATA_LEN];
static uint16_t g_read_datalen;
static int      g_charwr_result;
static int      g_charrd_result;
static int      g_descwr_result;
static int      g_descrd_result;
//static BLE_UUID g_target_srv_uuid;
//static BLE_UUID g_target_char_uuid;
#define DEVICE_NAME_LENGTH 64
static char     target_device_name[DEVICE_NAME_LENGTH] = {0};
static bool     mtu_updated;
static int      g_encrypted = -1;
static struct ble_gattc_db_disc_char_s g_nrw_char;
static volatile int    g_discovered = false;

NotifyPeripheralCallback notify_peripheral = nullptr;
WritePeripheralCallback write_peripheral = nullptr;
ReadPeripheralCallback read_peripheral = nullptr;

NotifyCentralCallback notify_central = nullptr;
WriteCentralCallback write_central = nullptr;
ReadCentralCallback read_central = nullptr;

#define RESULT_NOT_RECEIVE 1
#define INVALID_PARAMS     2

#define INVALID_CONN_HANDLE 0xffff

/****************************************************************************
 * static C Function Prototypes  BLE common callbacks
 ****************************************************************************/

/* Connection status change */
static void onLeConnectStatusChanged(struct ble_state_s *ble_state, bool connected, uint8_t reason);

/* Device name change */
static void onConnectedDeviceNameResp(const char *name);

/* Save bonding information */
static void onSaveBondInfo(int num, struct ble_bondinfo_s *bond);

/* Load bonding information */
static int onLoadBondInfo(int num, struct ble_bondinfo_s *bond);

/* Negotiated MTU size */
static void onMtuSize(uint16_t handle, uint16_t sz);

/* Encryption result */
static void onEncryptionResult(uint16_t, bool result);

/* Result callback for scan */
static void onScanResult(BT_ADDR addr, uint8_t *data, uint8_t len);

/****************************************************************************
 * static C Function Prototypes  BLE GATT callbacks
 ****************************************************************************/
/* For Peripheral */
/* Write request */
static void onPeripheralWrite(struct ble_gatt_char_s *ble_gatt_char);

/* Read request */
static void onPeripheralRead(struct ble_gatt_char_s *ble_gatt_char);

/* Notify request */
static void onPeripheralNotify(struct ble_gatt_char_s *ble_gatt_char, bool enable);

/* For Central */
/* Write request */
static void onCentralWrite(uint16_t conn_handle, struct ble_gatt_char_s *ble_gatt_char);

/* Read request */
static void onCentralRead(uint16_t conn_handle, struct ble_gatt_char_s *ble_gatt_char);

/* Notify request */
static void onCentralNotify(uint16_t conn_handle, struct ble_gatt_char_s *ble_gatt_char, bool enable);

/* DB discovery result */
static void onDiscovery(struct ble_gatt_event_db_discovery_t *db_disc);

/* Descriptor write response */
static void onDescriptorWrite(uint16_t, uint16_t, int);

/* Descriptor read response */
static void onDescriptorRead(uint16_t, uint16_t, uint8_t*, uint16_t);

/****************************************************************************
 * helper C Functions
 ****************************************************************************/
static void free_cccd(void);
static void show_uuid(BLE_UUID *uuid);
static void show_descriptor_handle(const char*, uint16_t);

/****************************************************************************
 * helper wrapper class definition
 ****************************************************************************/

BLE1507 *BLE1507::theInstance = nullptr;

BLE1507::BLE1507() {
  ble_common_ops.connect_status_changed     = onLeConnectStatusChanged;
  ble_common_ops.connected_device_name_resp = onConnectedDeviceNameResp;
  ble_common_ops.scan_result                = onScanResult;
  ble_common_ops.mtusize                    = onMtuSize;
  ble_common_ops.save_bondinfo              = onSaveBondInfo;
  ble_common_ops.load_bondinfo              = onLoadBondInfo;
  ble_common_ops.encryption_result          = onEncryptionResult;

  /* for peripheral */
  ble_gatt_peripheral_ops.write  = onPeripheralWrite;
  ble_gatt_peripheral_ops.read   = onPeripheralRead;
  ble_gatt_peripheral_ops.notify = onPeripheralNotify;

  attr_param.readPerm  = BLE_SEC_MODE1LV2_NO_MITM_ENC;
  attr_param.writePerm = BLE_SEC_MODE1LV2_NO_MITM_ENC;

  char_value.length = BLE_MAX_CHAR_SIZE;

  char_property.read         = 1;
  char_property.write        = 1;  
  char_property.writeWoResp  = 1;  
  char_property.notify       = 1;

  g_ble_gatt_char.handle = 0;
  g_ble_gatt_char.ble_gatt_peripheral_ops = &ble_gatt_peripheral_ops;

  /* for central */
  ble_gatt_central_ops.write              = onCentralWrite;
  ble_gatt_central_ops.read               = onCentralRead;
  ble_gatt_central_ops.notify             = onCentralNotify;
  ble_gatt_central_ops.database_discovery = onDiscovery;
  ble_gatt_central_ops.descriptor_write   = onDescriptorWrite;
  ble_gatt_central_ops.descriptor_read    = onDescriptorRead;

  mtu_updated = false;

}

BLE1507::~BLE1507() {
  free_cccd();
}

/****************************************************************************
 * beginPeripheral
 ****************************************************************************/
bool BLE1507::beginPeripheral(char *ble_name, BT_ADDR addr, uint32_t uuid_service, uint32_t uuid_char) {

  BLE_UUID *s_uuid;
  BLE_UUID *c_uuid;

  device_mode = blePeripheral;

  /* Initialize BT HAL */
  int ret = bt_init();
  if (ret != BT_SUCCESS) {
    printf("%s [BT] Initialization failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Register BLE common callbacks */
  ret = ble_register_common_cb(&ble_common_ops);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Register common call back failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Turn ON BT */
  ret = bt_enable();
  if (ret != BT_SUCCESS) {
    printf("%s [BT] Enabling failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Free memory that is allocated in onLoadBond() callback function. */
  free_cccd();

  /* BLE set name */
  ret = ble_set_name(ble_name);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Set name failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* BLE set address */
  ret = ble_set_address(&addr);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Set address failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* BLE enable */
  ret = ble_enable();
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Enable failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* BLE create GATT service instance */
  ret = ble_create_service(&g_ble_gatt_service);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Create GATT service failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Setup Service */
  /* Get Service UUID pointer */
  s_uuid = &g_ble_gatt_service->uuid;

  /* Setup Service UUID */
  s_uuid->type                  = BLE_UUID_TYPE_BASEALIAS_BTSIG;
  s_uuid->value.alias.uuidAlias = uuid_service;

  /* Setup Characteristic */

  /* Get Characteristic UUID pointer */
  c_uuid = &g_ble_gatt_char.uuid;

  /* Setup Characteristic UUID */
  c_uuid->type = BLE_UUID_TYPE_BASEALIAS_BTSIG;
  c_uuid->value.alias.uuidAlias = uuid_char;

  /* Set data point */
  char_value.data = char_data;

  /* Setup Characteristic BLE_ATTR_PERM */
  memcpy(&char_value.attrPerm, &attr_param, sizeof(BLE_ATTR_PERM));

  /* Setup Characteristic BLE_CHAR_VALUE */
  memcpy(&g_ble_gatt_char.value, &char_value, sizeof(BLE_CHAR_VALUE));

  /* Setup Characteristic BLE_CHAR_PROP */
  memcpy(&g_ble_gatt_char.property, &char_property, sizeof(BLE_CHAR_PROP));

  /* BLE add GATT characteristic into service */
  ret = ble_add_characteristic(g_ble_gatt_service, &g_ble_gatt_char);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Add GATT characteristic failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* BLE register GATT service */
  ret = ble_register_servce(g_ble_gatt_service);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Register GATT service failed. ret = %d\n", __func__, ret);
    return false;
  }

  return true;
}

/****************************************************************************
 * beginCentral
 ****************************************************************************/
bool BLE1507::beginCentral(char *ble_name, BT_ADDR addr) {

  device_mode = blePeripheral;

  /* Initialize BT HAL */
  int ret = bt_init();
  if (ret != BT_SUCCESS) {
    printf("%s [BT] Initialization failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Register BLE common callbacks */
  ret = ble_register_common_cb(&ble_common_ops);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Register common call back failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Turn ON BT */
  ret = bt_enable();
  if (ret != BT_SUCCESS) {
    printf("%s [BT] Enabling failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Free memory that is allocated in onLoadBond() callback function. */
  free_cccd();

  /* Register BLE gatt callbacks */

  ret = ble_register_gatt_central_cb(&ble_gatt_central_ops);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Register central call back failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* BLE set name */
  ret = ble_set_name(ble_name);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Set name failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* BLE set address */
  ret = ble_set_address(&addr);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Set address failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* BLE enable */
  ret = ble_enable();
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Enable failed. ret = %d\n", __func__, ret);
    return false;
  }

  return true;

}

/****************************************************************************
 * startAdvertise
 ****************************************************************************/
bool BLE1507::startAdvertise() 
{
  /* BLE start advertise */
  int ret = ble_start_advertise();
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Start advertise failed. ret = %d\n", __func__, ret);
    return false;
  }

  return true;
}


/****************************************************************************
 * startScan
 ****************************************************************************/
bool BLE1507::startScan(const char *val) 
{
  /* BLE start scan */
  int ret = ble_start_scan(false);
  if (ret != BT_SUCCESS) {
    printf("%s [BLE] Start scan failed. ret = %d\n", __func__, ret);
    return false;
  }

  if(strlen(val) >= DEVICE_NAME_LENGTH){
    puts("device name too long.");
    return false;
  }

  strcpy(target_device_name,val);

  return true;
}

/****************************************************************************
 * writeNotify
 ****************************************************************************/
bool BLE1507::writeNotify(uint8_t data[], uint32_t data_size) {
  if (is_ble_notify_enabled()) {
    int ret = ble_characteristic_notify(ble_conn_handle, &g_ble_gatt_char, data, data_size);
    if (ret != BT_SUCCESS) {
      printf("%s [BLE] Send data failed. ret = %d\n", __func__, ret);
      return false;
    }
    return true;
  }
  //printf("ble notify is not enabled\n");
  return false;
}

struct ble_gattc_db_disc_char_s* BLE1507::getCharacteristic()
{
  g_discovered = false;

/*  if (g_target)
    {
      ble_discover_uuid(conn_handle,
                        &g_target_srv_uuid,
                        &g_target_char_uuid);
    }
  else
    {*/
      ble_start_db_discovery(s_ble_state->ble_connect_handle);
//    }

  /* Wait for discovery result to be notified */

  while (g_discovered == false)
    {
      usleep(1); /* usleep for task dispatch */
    }

  return (g_nrw_char.characteristic.char_valhandle != 0) ? &g_nrw_char : NULL;
}

int BLE1507::pairing()
{
  int ret;

  g_encrypted = -1;
  ret = ble_pairing(s_ble_state->ble_connect_handle);
  if (ret != BT_SUCCESS)
    {
      return ret;
    }

  /* Wait for encryption result to be notified. */

  while (g_encrypted == -1)
    {
      usleep(1); /* usleep for task dispatch */
    }

  return (g_encrypted) ? BT_SUCCESS : BT_FAIL;
}


bool BLE1507::is_ble_notify_enabled() { 
  return ble_is_notify_enabled; 
}

void BLE1507::setNotifyPeripheralCallback(NotifyPeripheralCallback notify_cb) {
  notify_peripheral = notify_cb; 
}

void BLE1507::setWritePeripheralCallback(WritePeripheralCallback write_cb) { 
  write_peripheral = write_cb; 
}

void BLE1507::setReadPeripheralCallback(ReadPeripheralCallback read_cb) { 
  read_peripheral = read_cb; 
}

void BLE1507::setNotifyCentralCallback(NotifyCentralCallback notify_cb) {
  notify_central = notify_cb; 
}

void BLE1507::setWriteCentralCallback(WriteCentralCallback write_cb) { 
  write_central = write_cb; 
}

void BLE1507::setReadCentralCallback(ReadCentralCallback read_cb) { 
  read_central = read_cb; 
}


bool BLE1507::isMtuUpdated()
{
  return mtu_updated;
}


/****************************************************************************
 * remove bonding information
 ****************************************************************************/
bool BLE1507::removeBoundingInfo()
{
	return ((remove(BONDINFO_FILENAME)==0) ? true : false);
}

/****************************************************************************
 * finalize BLE
 ****************************************************************************/
/* Do not used now */
bool BLE1507::end()
{
  int ret;

  /* Turn OFF BT */
  ret = bt_disable();
  if (ret != BT_SUCCESS) {
    printf("%s [BT] BT disable failed. ret = %d\n", __func__, ret);
    return false;
  }

  /* Finalize BT */
  ret = bt_finalize();
  if (ret != BT_SUCCESS) {
    printf("%s [BT] BT finalize failed. ret = %d\n", __func__, ret);
    return false;
  }

  return true;
}

/****************************************************************************
 * static C Functions BLE common callbacks
 ****************************************************************************/

static void onLeConnectStatusChanged(struct ble_state_s *ble_state, bool connected, uint8_t reason) {

  BT_ADDR addr = ble_state->bt_target_addr;

  /* If receive connected status data, this function will call. */

  printf("[BLE_GATT] Connect status ADDR:%02X:%02X:%02X:%02X:%02X:%02X, status:%s, reason:%d\n",
          addr.address[5], addr.address[4], addr.address[3],
          addr.address[2], addr.address[1], addr.address[0],
          connected ? "Connected" : "Disconnected", reason);

  ble_conn_handle = ble_state->ble_connect_handle;
  ble_is_notify_enabled = false;

  s_ble_state = ble_state;

}

static void onConnectedDeviceNameResp(const char *name) {
  /* If receive connected device name data, this function will call. */
  printf("%s [BLE] Receive connected device name = %s\n", __func__, name);
}

static void onSaveBondInfo(int num, struct ble_bondinfo_s *bond) {

  int i;
  FILE *fp;
  int sz;

  /* 
   * In this example, save the parameter `num` and each members of
   * the parameter `bond` in order to the file.
   */
  fp = fopen(BONDINFO_FILENAME, "wb");
  if (fp == NULL) {
    printf("Error: could not create file %s\n", BONDINFO_FILENAME);
    return;
  }

  fwrite(&num, 1, sizeof(int), fp);

  for (i = 0; i < num; i++) {
    fwrite(&bond[i], 1, sizeof(struct ble_bondinfo_s), fp);

    /* Because only cccd is pointer member, save it individually. */
    sz = bond[i].cccd_num * sizeof(struct ble_cccd_s);
    fwrite(bond[i].cccd, 1, sz, fp);
  }

  fclose(fp);
}

static int onLoadBondInfo(int num, struct ble_bondinfo_s *bond) {

  int i;
  FILE *fp;
  int stored_num;
  int sz;
  int ret;

  fp = fopen(BONDINFO_FILENAME, "rb");
  if (fp == NULL) return 0;


  ret = fread(&stored_num, 1, sizeof(int), fp);
  if (ret != sizeof(int)) {
    printf("Error: could not load due to %s read error.\n", BONDINFO_FILENAME);
    fclose(fp);
    return 0;
  }

  g_ble_bonded_device_num = (stored_num < num) ? stored_num : num;
  sz = g_ble_bonded_device_num * sizeof(struct ble_cccd_s *);
  g_cccd = (struct ble_cccd_s **)malloc(sz);
  if (g_cccd == NULL) {
    printf("Error: could not load due to malloc error.\n");
    g_ble_bonded_device_num = 0;
  }

  for (i = 0; i < g_ble_bonded_device_num; i++) {
    ret = fread(&bond[i], 1, sizeof(struct ble_bondinfo_s), fp);
    if (ret != sizeof(struct ble_bondinfo_s)) {
      printf("Error: could not load all data due to %s read error.\n", BONDINFO_FILENAME);
      printf("The number of loaded device is %d\n", i);
      g_ble_bonded_device_num = i;
      break;
    }

    if (bond[i].cccd_num > 1) {
      printf("Error: could not load all data due to invalid data.\n");
      printf("cccd_num does not exceed the number of characteristics\n");
      printf("that is set by this application.\n");
      printf("The number of loaded device is %d\n", i);

      g_ble_bonded_device_num = i;
      break;
    }

    /* Because only cccd is pointer member, load it individually. */
    sz = bond[i].cccd_num * sizeof(struct ble_cccd_s);
    g_cccd[i] = (struct ble_cccd_s *)malloc(sz);

    if (g_cccd[i] == NULL) {
      printf("Error: could not load all data due to malloc error.");
      printf("The number of loaded device is %d\n", i);

      g_ble_bonded_device_num = i;
      break;
    }

    bond[i].cccd = g_cccd[i];
    ret = fread(bond[i].cccd, 1, sz, fp);
    if (ret != sz) {
      printf("Error: could not load all data due to %s read error.\n", BONDINFO_FILENAME);
      printf("The number of loaded device is %d\n", i);
      g_ble_bonded_device_num = i;
      break;
    }
  }

  fclose(fp);
  return g_ble_bonded_device_num;
}


static void onMtuSize(uint16_t handle, uint16_t sz) {
  printf("negotiated MTU size(connection handle = %d) : %d\n", handle, sz);

  mtu_updated = true;
}

static void onEncryptionResult(uint16_t handle, bool result) {
  printf("Encryption result(connection handle = %d) : %s\n",
         handle, (result) ? "Success" : "Fail");
  g_encrypted = result;

}

static void onScanResult(BT_ADDR addr, uint8_t *data, uint8_t len)
{
  struct ble_state_s state = {0};
  int ret = BT_SUCCESS;
  char devname[BT_EIR_LEN];

  devname[0] = '\0';
//  printf("[BLE] Scan result ADDR:%02X:%02X:%02X:%02X:%02X:%02X (RSSI:%ddB)\n",
  printf("[BLE] Scan result ADDR:%02X:%02X:%02X:%02X:%02X:%02X\n",
           addr.address[5], addr.address[4], addr.address[3],
//         addr.address[2], addr.address[1], addr.address[0],
//         bleutil_get_rssi(data, len));
         addr.address[2], addr.address[1], addr.address[0]);

  /* If peer device has the device name, print it. */

  if (bleutil_get_devicename(data, len, devname))
    {
      printf("[BLE] Scan device name: %s\n", devname);
    }

/*  if (bleutil_find_srvc_uuid(&g_target_srv_uuid, data, len) == 0)
    {
      printf("Service UUID is not matched\n");
      return;
    }*/

  if (target_device_name != 0) {
    if (strcmp((char *)devname, target_device_name) != 0)
      {
        printf("%s [BLE] DevName is not matched: %s expect: %s\n",
               __func__, devname, target_device_name);
        return;
      }
  }

  ret = ble_cancel_scan();
  if (ret != BT_SUCCESS)
    {
      printf("%s [BLE_GATT] Cancel scan failed. ret = %d\n", __func__, ret);
      return;
    }

  bleutil_add_btaddr(&state, &addr, bleutil_get_addrtype(data, len));

  ret = ble_connect(&state);
  if (ret != BT_SUCCESS)
    {
      printf("%s [BLE_GATT] connect target device failed. ret = %d\n", __func__, ret);
      return;
    }
  else
    {
      printf("%s [BLE_GATT] connect target device. ret = %d\n", __func__, ret);
    }

  return;
}

/****************************************************************************
 * Event callbacks
 ****************************************************************************/
/* For Peripheral */
static void onPeripheralWrite(struct ble_gatt_char_s *ble_gatt_char) {

#ifdef PRINT_DEBUG
  printf("%s [BLE] start\n", __func__);

  printf("handle : %d\n", ble_gatt_char->handle);
  show_uuid(&ble_gatt_char->uuid);
  printf("value_len : %d\n", ble_gatt_char->value.length);
  printf("value : ");
  for (int i = 0; i < ble_gatt_char->value.length; i++) {
    printf("%02x ", ble_gatt_char->value.data[i]);
  }
  printf("\n");
#endif

  if (write_peripheral != nullptr) {
    write_peripheral(ble_gatt_char);
  }

#ifdef PRINT_DEBUG
  printf("%s [BLE] end\n", __func__);
#endif
}

static void onPeripheralRead(struct ble_gatt_char_s *ble_gatt_char) {

#ifdef PRINT_DEBUG
  printf("%s [BLE] start\n", __func__);
#endif

  if (read_peripheral != nullptr) {
    read_peripheral(ble_gatt_char);
  }

#ifdef PRINT_DEBUG
  printf("%s [BLE] end \n", __func__);
#endif
}

static void onPeripheralNotify(struct ble_gatt_char_s *ble_gatt_char, bool enable) {

#ifdef PRINT_DEBUG
  printf("%s [BLE] start \n", __func__);
  printf("handle : %d\n", ble_gatt_char->handle);
  show_uuid(&ble_gatt_char->uuid);
#endif

  if (enable) {
    printf("notification enabled\n");
    ble_is_notify_enabled = true;
  } else {
    printf("notification disabled\n");
    ble_is_notify_enabled = false;
  }

  if (notify_peripheral != nullptr) {
    notify_peripheral(ble_gatt_char);
  }

#ifdef PRINT_DEBUG
  printf("%s [BLE] end \n", __func__);
#endif
}

/* For Central */
static void onCentralWrite(uint16_t conn_handle, struct ble_gatt_char_s *ble_gatt_char) {

#ifdef PRINT_DEBUG
  printf("%s [BLE] start\n", __func__);

  printf("handle : %d\n", ble_gatt_char->handle);
  show_uuid(&ble_gatt_char->uuid);
  printf("value_len : %d\n", ble_gatt_char->value.length);
  printf("value : ");
  for (int i = 0; i < ble_gatt_char->value.length; i++) {
    printf("%02x ", ble_gatt_char->value.data[i]);
  }
  printf("\n");
#endif

  if (write_central != nullptr) {
    write_central(conn_handle, ble_gatt_char);
  }

#ifdef PRINT_DEBUG
  printf("%s [BLE] end\n", __func__);
#endif
}

static void onCentralRead(uint16_t conn_handle, struct ble_gatt_char_s *ble_gatt_char) {

#ifdef PRINT_DEBUG
  printf("%s [BLE] start\n", __func__);
#endif

  if (read_central != nullptr) {
    read_central(conn_handle, ble_gatt_char);
  }

#ifdef PRINT_DEBUG
  printf("%s [BLE] end \n", __func__);
#endif
}

static void onCentralNotify(unsigned short conn_handle, struct ble_gatt_char_s *ble_gatt_char, bool enable) {

#ifdef PRINT_DEBUG
  printf("%s [BLE] start \n", __func__);
  printf("handle : %d\n", ble_gatt_char->handle);
  show_uuid(&ble_gatt_char->uuid);
#endif

  if (enable)
    {
      printf("   notification enabled\n");
    }
  else
    {
      printf("   notification disabled\n");
    }

  if (notify_central != nullptr) {
    notify_central(conn_handle, ble_gatt_char);
  }

#ifdef PRINT_DEBUG
  printf("%s [BLE] end \n", __func__);
#endif
}

static void onDiscovery(struct ble_gatt_event_db_discovery_t *db_disc)
{

#ifdef PRINT_DEBUG
  printf("%s [BLE] start\n", __func__);
#endif

  int i;
  int j;
  struct ble_gattc_db_discovery_s *db;
  struct ble_gattc_db_disc_srv_s  *srv;
  struct ble_gattc_db_disc_char_s *ch;

  db = &db_disc->params.db_discovery;
  srv = &db->services[0];

  for (i = 0; i < db->srv_count; i++, srv++)
    {
      printf("=== SRV[%d] ===\n", i);

      show_uuid(&srv->srv_uuid);

      ch = &srv->characteristics[0];

      for (j = 0; j < srv->char_count; j++, ch++)
        {
          printf("   === CHR[%d] ===\n", j);
          printf("      decl  handle : 0x%04x\n", ch->characteristic.char_declhandle);
          printf("      value handle : 0x%04x\n", ch->characteristic.char_valhandle);
          show_uuid(&ch->characteristic.char_valuuid);

          show_descriptor_handle("cccd", ch->cccd_handle);
          show_descriptor_handle("cepd", ch->cepd_handle);
          show_descriptor_handle("cudd", ch->cudd_handle);
          show_descriptor_handle("sccd", ch->sccd_handle);
          show_descriptor_handle("cpfd", ch->cpfd_handle);
          show_descriptor_handle("cafd", ch->cafd_handle);
          printf("\n");

          if (ch->cccd_handle != BLE_GATT_INVALID_ATTRIBUTE_HANDLE)
            {
              last_cccd_handle = ch->cccd_handle;
            }
        }

      printf("\n");
    }

  /* The end_handle of the last service is 0xFFFF.
   * So, if end_handle is not 0xFFFF, the continuous service information exists.
   */

  if (db_disc->state.end_handle != 0xFFFF)
    {
//      if (g_target == false)
        {
          ble_continue_db_discovery(db_disc->state.end_handle + 1,
                                    s_ble_state->ble_connect_handle);
        }
    }
	puts("g_discovered = true");
  g_discovered = true;
	
//ble_pairing(s_ble_state->ble_connect_handle);

#ifdef PRINT_DEBUG
  printf("%s [BLE] end \n", __func__);
#endif
}

static void onDescriptorWrite(uint16_t conn_handle, uint16_t handle, int result)
{
#ifdef PRINT_DEBUG
  printf("%s [BLE] start\n", __func__);

  printf("%s [BLE] conn_handle = 0x%04x, handle = 0x%04x, result = %d\n",
         __func__, conn_handle, handle, result);
#endif

  g_descwr_result = result;

#ifdef PRINT_DEBUG
  printf("%s [BLE] end \n", __func__);
#endif

}

static void onDescriptorRead(uint16_t conn_handle, uint16_t handle,
                               uint8_t  *data, uint16_t len)
{
#ifdef PRINT_DEBUG
  printf("%s [BLE] start\n", __func__);

  printf("conn_handle : 0x%04x, handle : 0x%04x\n", conn_handle, handle);
  printf("len : %d\n",  len);

  for (int i = 0; i < len; i++)
    {
      printf("%02x ", data[i]);
    }

  printf("\n");

#endif

  g_read_datalen = len;
  memcpy(g_read_data, data, g_read_datalen);
  g_descrd_result = (len) ? BT_SUCCESS : BT_FAIL;

#ifdef PRINT_DEBUG
  printf("%s [BLE] end \n", __func__);
#endif

}

/****************************************************************************
  Descriptor control
 ****************************************************************************/

int BLE1507::writeDescriptor(uint16_t conn_handle,
                            uint16_t desc_handle,
                            uint8_t  *buf,
                            uint16_t len)
{
  int ret;

  g_descwr_result = RESULT_NOT_RECEIVE;

  ret = ble_descriptor_write(conn_handle, desc_handle, buf, len);
  if (ret != BT_SUCCESS)
    {
      return ret;
    }

  /* Wait for write response. */

  while (g_descwr_result == RESULT_NOT_RECEIVE)
    {
      usleep(1); /* usleep for task dispatch */
    }

  return g_descwr_result;
}

int BLE1507::readDescriptor(uint16_t conn_handle,
                             uint16_t desc_handle,
                             uint8_t  *buf,
                             uint16_t *len)
{
  int ret;

  g_descrd_result = RESULT_NOT_RECEIVE;
  ret = ble_descriptor_read(conn_handle, desc_handle);
  if (ret != BT_SUCCESS)
    {
      return ret;
    }

  /* Wait for read response. */

  while (g_descrd_result == RESULT_NOT_RECEIVE)
    {
      usleep(1);
    }

  if (g_descrd_result == BT_SUCCESS)
    {
      *len = g_read_datalen;
      memcpy(buf, g_read_data, g_read_datalen);
    }

  return g_descrd_result;
}

/****************************************************************************
  Characteristic control
 ****************************************************************************/
int BLE1507::writeCharacteristic(uint16_t conn_handle,
                                uint16_t char_handle,
                                uint8_t  *buf,
                                uint16_t len,
                                bool     rsp)
{
  int ret;

  g_charwr_result = RESULT_NOT_RECEIVE;

  ret = ble_write_characteristic(conn_handle, char_handle, buf, len, rsp);
  if (ret != BT_SUCCESS)
    {
      return ret;
    }

  if (!rsp)
    {
      /* In write w/o response case, not wait response. */

      return OK;
    }

  /* Wait for write response. */

  while (g_charwr_result == RESULT_NOT_RECEIVE)
    {
      usleep(1); /* usleep for task dispatch */
    }

  return g_charwr_result;
}

int BLE1507::readCharacteristic(uint16_t conn_handle,
                               uint16_t char_handle,
                               uint8_t  *buf,
                               uint16_t *len)
{
  int ret;

  g_charrd_result = RESULT_NOT_RECEIVE;

  ret = ble_read_characteristic(conn_handle, char_handle);
  if (ret != BT_SUCCESS)
    {
      return ret;
    }

  /* Wait for read response. */

  while (g_charrd_result == RESULT_NOT_RECEIVE)
    {
      usleep(1); /* usleep for task dispatch */
    }

  if (g_charrd_result == BT_SUCCESS)
    {
      *len = g_read_datalen;
      memcpy(buf, g_read_data, g_read_datalen);
    }

  return g_charrd_result;
}

/****************************************************************************
 * helper C Functions
 ****************************************************************************/
static void show_uuid(BLE_UUID *uuid) {
  int i;
  printf("uuid : ");

  switch (uuid->type) {
  case BLE_UUID_TYPE_UUID128:

    /* UUID format YYYYYYYY-YYYY-YYYY-YYYY-YYYYYYYYYYYY */
    for (i = 0; i < BT_UUID128_LEN; i++) {
      printf("%02x", uuid->value.uuid128.uuid128[BT_UUID128_LEN - i - 1]);
      if ((i == 3) || (i == 5) || (i == 7) || (i == 9)) printf("-");
    }
    printf("\n");
    break;

  case BLE_UUID_TYPE_BASEALIAS_BTSIG:
  case BLE_UUID_TYPE_BASEALIAS_VENDOR:

    /* UUID format XXXX */
    printf("%04x\n", uuid->value.alias.uuidAlias);
    break;

  default:
    for (i = 0; i < BT_UUID128_LEN; i++) {
      printf("%02x", uuid->value.uuid128.uuid128[BT_UUID128_LEN - i - 1]);
      if ((i == 3) || (i == 5) || (i == 7) || (i == 9)) printf("-");
    }
    printf("\n");
//    printf("%04x\n", uuid->value.alias.uuidAlias);
    printf("Irregular UUID type.\n");
    break;
  }
}

static void show_descriptor_handle(const char *name, uint16_t handle)
{
  if (handle == BLE_GATT_INVALID_ATTRIBUTE_HANDLE)
    {
      return;
    }

  printf("      %s  handle : 0x%04x\n", name, handle);
}

static void free_cccd(void) {
  int i;

  if (g_cccd) {
    for (i = 0; i < g_ble_bonded_device_num; i++) {
      if (g_cccd[i]) free(g_cccd[i]);
    }

    free(g_cccd);
    g_cccd = NULL;
  }
}

