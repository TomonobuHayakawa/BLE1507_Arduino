/*
 * Notify sample on BLE central
 *
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "BLE1507.h"

static BT_ADDR addr               = {{0x19, 0x84, 0x06, 0x14, 0xAB, 0xCD}};
static char ble_name[BT_NAME_LEN] = "SPR-CENTRAL";

BLE1507 *ble1507;

/****************************************************************************
 * ble callbacks
 ****************************************************************************/
void bleNotifyCB(uint16_t conn_handle, struct ble_gatt_char_s *ble_gatt_char) {
  printf("notify_callback!");
  printf("length : %d \n",ble_gatt_char->value.length);
  printf("value : ");
  for (int i = 0; i < ble_gatt_char->value.length; i++) {
    printf("%x ", ble_gatt_char->value.data[i]);
  }
  printf("\n");

}
/****************************************************************************
 * setup function
 ****************************************************************************/
void setup()
{
  struct ble_gattc_db_disc_char_s *nrw_char;

  ble1507 = BLE1507::getInstance();
//  ble1507->removeBoundingInfo();
  ble1507->beginCentral(ble_name, addr);
  ble1507->setNotifyCentralCallback(&bleNotifyCB);

  ble1507->startScan("SPR-PERIPHERAL");

  while(!ble1507->isMtuUpdated());
  nrw_char = ble1507->getCharacteristic();

  ble1507->pairing();
}

void loop()
{
  sleep(100);
}
