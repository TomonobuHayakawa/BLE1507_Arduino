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
struct ble_gattc_db_disc_char_s* nrw_char;

/****************************************************************************
 * setup function
 ****************************************************************************/
void setup()
{
  ble1507 = BLE1507::getInstance();
//  ble1507->removeBoundingInfo();
  ble1507->beginCentral(ble_name, addr);

  ble1507->startScan("SPR-PERIPHERAL");

  while(!ble1507->isMtuUpdated());
  nrw_char = ble1507->getCharacteristic();

  ble1507->pairing();
}

void loop()
{
  const int len = 4;
  uint8_t buf[len];
  static int data = 0;

  for(int i=0;i<len;i++) buf[i] = (0xff & data >> (8*i)) ;
  ble1507->writeCharacteristic(nrw_char->characteristic.char_valhandle, buf, len, false);
  data++;
  sleep(1);
}
