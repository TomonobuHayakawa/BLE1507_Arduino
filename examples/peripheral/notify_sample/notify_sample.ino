/****************************************************************************
 * Notify sample on BLE Peripheral
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "BLE1507.h"

/****************************************************************************
 * ble parameters
 ****************************************************************************/
#define UUID_SERVICE  0x3802
#define UUID_CHAR     0x4a02

static BT_ADDR addr = {{0x19, 0x84, 0x06, 0x14, 0xAB, 0xCD}};
static char ble_name[BT_NAME_LEN] = "SPR-PERIPHERAL";

BLE1507 *ble1507;

/****************************************************************************
 * ble callbacks
 ****************************************************************************/
void bleWriteCB(struct ble_gatt_char_s *ble_gatt_char) {
  printf("write_callback!");
  printf("value : ");
  for (int i = 0; i < ble_gatt_char->value.length; i++) {
    printf("%c", ble_gatt_char->value.data[i]);
  }
  printf("\n");

}

/****************************************************************************
 * setup function
 ****************************************************************************/
void setup() {
  ble1507 = BLE1507::getInstance();
  ble1507->beginPeripheral(ble_name, addr, UUID_SERVICE, UUID_CHAR);
  ble1507->setWritePeripheralCallback(bleWriteCB);

  ble1507->startAdvertise();
}

/****************************************************************************
 * loop function
 ****************************************************************************/
void loop() {

  static uint8_t ble_notify_data = 0x30;
  const int digit_num_size = 4;
  uint8_t data[digit_num_size] = {0};

  /*
   * Transmit one byte data once per second.
   * The data increases by one for each trasmission.
   */

  for(int i = 0; i < digit_num_size; i++) {
    data[i] = ble_notify_data++;
    if (ble_notify_data == 0x39) ble_notify_data = 0x30; 
  }
  
  ble1507->writeNotify(data, digit_num_size);

  sleep(1);

}
