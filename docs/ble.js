const BLE_SERVICE_UUID = '12345678-1234-5678-1234-56789abcdef0';
const BLE_CONFIG_UUID  = '12345678-1234-5678-1234-56789abcdef1';
const BLE_COMMAND_UUID = '12345678-1234-5678-1234-56789abcdef2';

window.BleConfig = (() => {
  let device = null;
  let configCharacteristic = null;
  let commandCharacteristic = null;

  function status(message) {
    const el = document.getElementById('status');
    if (el) el.textContent = message;
  }

  async function connect(type) {
    const save = document.getElementById('save');
    if (!navigator.bluetooth) {
      status('Web Bluetooth is not available in this browser. Open this page in Bluefy.');
      return;
    }

    try {
      status('Choose the ' + type + ' unit…');

      device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [BLE_SERVICE_UUID] }],
        optionalServices: [BLE_SERVICE_UUID]
      });

      device.addEventListener('gattserverdisconnected', () => {
        status('Disconnected from ' + (device.name || 'BLE device') + '.');
        if (save) save.disabled = true;
      });

      const server = await device.gatt.connect();
      const service = await server.getPrimaryService(BLE_SERVICE_UUID);

      configCharacteristic = await service.getCharacteristic(BLE_CONFIG_UUID);
      commandCharacteristic = await service.getCharacteristic(BLE_COMMAND_UUID);

      status('Connected to ' + (device.name || 'BLE device') + '.');

      if (configCharacteristic.properties.read) {
        const value = await configCharacteristic.readValue();
        const text = new TextDecoder().decode(value);
        if (text) loadJson(text);
      }

      if (save) save.disabled = false;
    } catch (err) {
      status('BLE connection failed: ' + err.message);
    }
  }

  function loadJson(text) {
    try {
      const cfg = JSON.parse(text);
      Object.entries(cfg).forEach(([key, value]) => {
        const el = document.getElementById(key);
        if (el) el.value = value;
      });
    } catch (err) {
      status('Connected, but the device returned invalid configuration data.');
    }
  }

  async function save(fields) {
    if (!configCharacteristic) {
      status('Connect to the unit first.');
      return;
    }

    const obj = {};
    fields.forEach(([id]) => {
      const el = document.getElementById(id);
      obj[id] = el && el.type === 'number' ? Number(el.value) : el.value;
    });

    try {
      const bytes = new TextEncoder().encode(JSON.stringify(obj));
      await configCharacteristic.writeValue(bytes);
      status('Configuration saved to the unit. Rebooting…');

      if (commandCharacteristic) {
        await commandCharacteristic.writeValue(
          new TextEncoder().encode('REBOOT')
        );
      }
    } catch (err) {
      status('Configuration write failed: ' + err.message);
    }
  }

  return { connect, save };
})();