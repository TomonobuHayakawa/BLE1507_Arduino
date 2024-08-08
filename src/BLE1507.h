#ifndef BLE1507_HEADER_GUARD__
#define BLE1507_HEADER_GUARD__

#include <bluetooth/ble_gatt.h>
#include <bluetooth/ble_util.h>
#include <bluetooth/hal/bt_if.h>

#define BONDINFO_FILENAME "/mnt/spif/BONDINFO"


#define NOTIFICATION_ENABLED 1
#define NOTIFICATION_DISABLED 0

typedef void (*NotifyPeripheralCallback)(struct ble_gatt_char_s*);
typedef void (*WritePeripheralCallback)(struct ble_gatt_char_s*);
typedef void (*ReadPeripheralCallback)(struct ble_gatt_char_s*);

typedef void (*NotifyCentralCallback)(uint16_t conn_handle, struct ble_gatt_char_s*);
typedef void (*WriteCentralCallback)(uint16_t conn_handle, struct ble_gatt_char_s*);
typedef void (*ReadCentralCallback)(uint16_t conn_handle, struct ble_gatt_char_s*);


/****************************************************************************
 * helper wrapper class definition
 ****************************************************************************/
class BLE1507 {
  private:
    BLE1507();
    ~BLE1507();
  public:
    static BLE1507* getInstance() {
      if (theInstance == nullptr) {
        theInstance = new BLE1507();
      }
      return theInstance;
    }
    bool beginPeripheral(char *ble_name, BT_ADDR addr, uint32_t uuid_service, uint32_t uuid_char);
    bool beginCentral(char *ble_name, BT_ADDR addr);
    bool writeNotify(uint8_t data[], uint32_t data_size);
    bool is_ble_notify_enabled();

    void setNotifyPeripheralCallback(NotifyPeripheralCallback notify_cb);
    void setWritePeripheralCallback(WritePeripheralCallback write_cb);
    void setReadPeripheralCallback(ReadPeripheralCallback read_cb);
    void setNotifyCentralCallback(NotifyCentralCallback notify_cb);
    void setWriteCentralCallback(WriteCentralCallback write_cb);
    void setReadCentralCallback(ReadCentralCallback read_cb);

    bool startAdvertise();
    bool startScan(const char*);
    bool removeBoundingInfo();
    bool isMtuUpdated();
    struct ble_gattc_db_disc_char_s* getCharacteristic();
    int  pairing();

    int writeDescriptor(uint16_t, uint8_t*, uint16_t);
    int readDescriptor(uint16_t, uint8_t*, uint16_t*);
    int writeCharacteristic(uint16_t, uint8_t*, uint16_t, bool);
    int readCharacteristic(uint16_t, uint8_t*, uint16_t*);

    int writeDescriptor(uint16_t, uint16_t, uint8_t*, uint16_t);
    int readDescriptor(uint16_t, uint16_t, uint8_t*, uint16_t*);
    int writeCharacteristic(uint16_t, uint16_t, uint8_t*, uint16_t, bool);
    int readCharacteristic(uint16_t, uint16_t, uint8_t*, uint16_t*);

    bool end();

  private:

    typedef enum e_bleMode {
         bleCentral = 0,
         blePeripheral
    } bleMode;

    static BLE1507 *theInstance;
    bleMode device_mode;

};


#endif // BLE1507_HEADER_GUARD__