/*
 * BLE1507_write_and_notify.ino
 * Copyright (c) 2024 Yoshinori Oota
 *
 * This is an example of BLE1507
 *
 * This is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


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

static BT_ADDR addr = {{0x20, 0x84, 0x06, 0x14, 0xAB, 0xCD}};
static char ble_name[BT_NAME_LEN] = "SPR-PERIPHERAL";

BLE1507 *ble1507;


/****************************************************************************
 * ble write callback function
 ****************************************************************************/

void bleWriteCB(struct ble_gatt_char_s *ble_gatt_char) {
  static uint8_t data = 0;
  uint8_t str_data[4] = {0};
  if (ble_gatt_char->value.length == 0) return;
  if (ble_gatt_char->value.data[0] == 'Z') {
    printf("write_callback!");
    printf("raise notify to the central : ");
    sprintf((char*)str_data, "%03d", data++);
    data %= 100;
    ble1507->writeNotify(str_data, 3);
  }
  printf("%s\n", str_data);
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
  // no implementation
}