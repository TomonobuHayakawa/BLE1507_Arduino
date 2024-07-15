#ifndef BLE1507_HEADER_GUARD__
#define BLE1507_HEADER_GUARD__

#include <bluetooth/ble_gatt.h>

#define BONDINFO_FILENAME "/mnt/spif/BONDINFO"

typedef void (*NotifyCallback)(struct ble_gatt_char_s*);
typedef void (*WriteCallback)(struct ble_gatt_char_s*);
typedef void (*ReadCallback)(struct ble_gatt_char_s*);

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
    bool begin(char *ble_name, BT_ADDR addr, uint32_t uuid_service, uint32_t uuid_char);
    bool writeNotify(uint8_t data[], uint32_t data_size);
    bool is_ble_notify_enabled();
    void setNotifyCallback(NotifyCallback notify_cb);
    void setWriteCallback(WriteCallback write_cb);
    void setReadCallback(ReadCallback read_cb);

  private:
    static BLE1507 *theInstance;
};




#endif // BLE1507_HEADER_GUARD__