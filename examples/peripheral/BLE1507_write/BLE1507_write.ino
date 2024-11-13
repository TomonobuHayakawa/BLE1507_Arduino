/*
 * BLE1507_write.ino
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

static BT_ADDR addr = {{0x19, 0x84, 0x06, 0x14, 0xAB, 0xCD}};
static char ble_name[BT_NAME_LEN] = "SPR-PERIPHERAL";

BLE1507 *ble1507;


/****************************************************************************
 * ble write callback function
 ****************************************************************************/

void bleWriteCB(struct ble_gatt_char_s *ble_gatt_char) {
  static int led = 0;
  printf("write_callback!\n");
  ledOff(led);
  printf("length : %d \n",ble_gatt_char->value.length);
  printf("value : ");
  for (int i = 0; i < ble_gatt_char->value.length; i++) {
    printf("%c ", ble_gatt_char->value.data[i]);
  }
  led = (ble_gatt_char->value.data[0] % 4)+ 0x40;
  ledOn(led);
  printf("\n");

}

void bleCommandCB(struct ble_gatt_char_s *ble_gatt_char) {
  String command = "";
  for (int i = 0; i < ble_gatt_char->value.length; i++) {
    command += (char)ble_gatt_char->value.data[i];
  }
  Serial.println(command);

  if (command.equals("start")) {
    Serial.println("Recieve: start!");
  }
  else if (command.equals("stop")) {
    Serial.println("Recieve: stop!");
  }
  else {
    Serial.println("Recieve unknown command.");
  }
}
/****************************************************************************
 * setup function
 ****************************************************************************/
void setup() {

  Serial.begin(115200);

  ble1507 = BLE1507::getInstance();
  ble1507->beginPeripheral(ble_name, addr, UUID_SERVICE, UUID_CHAR);
  ble1507->setWritePeripheralCallback(bleWriteCB);
//  ble1507->removeBoundingInfo();

  ble1507->startAdvertise();
}

/****************************************************************************
 * loop function
 ****************************************************************************/
void loop() {

}
