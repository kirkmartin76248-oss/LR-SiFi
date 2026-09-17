// BLE commissioning interface for the LR-SiFi project.
// The ESP32 firmware will expose this custom service/characteristic.
const BLE_SERVICE_UUID = '12345678-1234-5678-1234-56789abcdef0';
const BLE_CONFIG_UUID  = '12345678-1234-5678-1234-56789abcdef1';

window.BleConfig = (() => {
  let device = null;
  let characteristic = null;

  async function connect(type) {
    const status = document.getElementById('status');
    const save = document.getElementById('save');
    if (!navigator.bluetooth) {
      status.textContent = 'This browser does not support Web Bluetooth. Use a compatible browser/device for BLE commissioning.';
      return;
    }
    try {
      status.textContent = 'Choose the ' + type + ' unit…';
      device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [BLE_SERVICE_UUID] }],
        optionalServices: [BLE_SERVICE_UUID]
      });
      const server = await device.gatt.connect();
      const service = await server.getPrimaryService(BLE_SERVICE_UUID);
      characteristic = await service.getCharacteristic(BLE_CONFIG_UUID);
      status.textContent = 'Connected to ' + (device.name || 'BLE device') + '.';
      save.disabled = false;
      if (characteristic.properties.read) {
        const value = await characteristic.readValue();
        const text = new TextDecoder().decode(value);
        if (text) loadJson(text);
      }
    } catch (err) {
      status.textContent = 'BLE connection cancelled or failed: ' + err.message;
    }
  }

  function loadJson(text) {
    try {
      const cfg = JSON.parse(text);
      Object.entries(cfg).forEach(([key, value]) => {
        const el = document.getElementById(key);
        if (el) el.value = value;
      });
    } catch (_) {
      // Firmware may return a future binary/alternate representation.
    }
  }

  async function save(fields) {
    const status = document.getElementById('status');
    if (!characteristic) return;
    const obj = {};
    fields.forEach(([id, value]) => {
      const el = document.getElementById(id);
      obj[id] = el && el.type === 'number' ? Number(value) : value;
    });
    try {
      const bytes = new TextEncoder().encode(JSON.stringify(obj));
      await characteristic.writeValue(bytes);
      status.textContent = 'Configuration written successfully.';
    } catch (err) {
      status.textContent = 'Configuration write failed: ' + err.message;
    }
  }

  return { connect, save };
})();
