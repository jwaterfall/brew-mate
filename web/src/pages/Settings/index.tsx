import { useState, useEffect } from "preact/hooks";
import { Save, Loader2, Wifi, WifiOff, Battery } from "lucide-preact";
import { apiUrl } from "../../utils/api";

interface WiFiConfig {
  ssid: string;
  password: string;
}

interface WiFiStatus {
  connected: boolean;
  ssid?: string;
  ip?: string;
  apMode: boolean;
}

interface BatteryCalibration {
  batteryCalibrationFactor: number;
  vbusCalibrationFactor: number;
}

export function Settings() {
  const [wifiConfig, setWifiConfig] = useState<WiFiConfig>({
    ssid: "",
    password: "",
  });
  const [wifiStatus, setWifiStatus] = useState<WiFiStatus>({
    connected: false,
    apMode: true,
  });
  const [isLoading, setIsLoading] = useState(true);
  const [isSaving, setIsSaving] = useState(false);
  const [saveMessage, setSaveMessage] = useState<{
    type: "success" | "error";
    text: string;
  } | null>(null);

  const [batteryCalibration, setBatteryCalibration] =
    useState<BatteryCalibration>({
      batteryCalibrationFactor: 1.0,
      vbusCalibrationFactor: 1.0,
    });
  const [isLoadingBattery, setIsLoadingBattery] = useState(true);
  const [isSavingBattery, setIsSavingBattery] = useState(false);
  const [batterySaveMessage, setBatterySaveMessage] = useState<{
    type: "success" | "error";
    text: string;
  } | null>(null);

  const fetchWiFiStatus = async () => {
    try {
      const response = await fetch(apiUrl("/api/wifi/status"));
      if (response.ok) {
        const data = await response.json();
        setWifiStatus(data);
      }
    } catch (error) {
      console.error("Failed to fetch WiFi status:", error);
    }
  };

  const loadWiFiConfig = async () => {
    try {
      const response = await fetch(apiUrl("/api/wifi/config"));
      if (response.ok) {
        const data = await response.json();
        setWifiConfig({
          ssid: data.ssid || "",
          password: "", // Don't load password for security
        });
      }
    } catch (error) {
      console.error("Failed to load WiFi config:", error);
    } finally {
      setIsLoading(false);
    }
  };

  const saveWiFiConfig = async () => {
    if (!wifiConfig.ssid.trim()) {
      setSaveMessage({
        type: "error",
        text: "SSID cannot be empty",
      });
      return;
    }

    setIsSaving(true);
    setSaveMessage(null);

    try {
      const response = await fetch(apiUrl("/api/wifi/config"), {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(wifiConfig),
      });

      const result = await response.json();

      if (response.ok && result.success) {
        setSaveMessage({
          type: "success",
          text: "WiFi credentials saved. The device will reconnect on next restart.",
        });
        // Clear password field after saving
        setWifiConfig({ ...wifiConfig, password: "" });
        // Refresh status after a delay
        setTimeout(() => {
          fetchWiFiStatus();
        }, 2000);
      } else {
        setSaveMessage({
          type: "error",
          text: result.error || "Failed to save WiFi credentials",
        });
      }
    } catch (error) {
      setSaveMessage({
        type: "error",
        text: "Failed to save WiFi credentials",
      });
    } finally {
      setIsSaving(false);
    }
  };

  useEffect(() => {
    loadWiFiConfig();
    fetchWiFiStatus();
    const interval = setInterval(fetchWiFiStatus, 5000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div class="min-h-screen">
      <div class="max-w-lg mx-auto py-4 px-4 sm:px-6 lg:px-8 space-y-6">
        <div>
          <h2 class="text-2xl font-bold mb-2">Settings</h2>
          <p class="text-slate-400">Configure your device settings</p>
        </div>

        {/* WiFi Status Card */}
        <div class="bg-slate-800 rounded-2xl p-6 shadow-lg">
          <div class="flex items-center justify-between mb-4">
            <h3 class="text-lg font-semibold flex items-center gap-2">
              {wifiStatus.connected ? (
                <Wifi class="w-5 h-5 text-green-400" />
              ) : (
                <WifiOff class="w-5 h-5 text-slate-400" />
              )}
              WiFi Status
            </h3>
          </div>

          <div class="space-y-2 text-sm">
            <div class="flex justify-between">
              <span class="text-slate-400">Status:</span>
              <span
                class={
                  wifiStatus.connected
                    ? "text-green-400 font-semibold"
                    : "text-slate-400"
                }
              >
                {wifiStatus.connected ? "Connected" : "Not Connected"}
              </span>
            </div>
            {wifiStatus.apMode && (
              <div class="flex justify-between">
                <span class="text-slate-400">Mode:</span>
                <span class="text-yellow-400 font-semibold">Access Point</span>
              </div>
            )}
            {wifiStatus.ssid && (
              <div class="flex justify-between">
                <span class="text-slate-400">Network:</span>
                <span class="text-white">{wifiStatus.ssid}</span>
              </div>
            )}
            {wifiStatus.ip && (
              <div class="flex justify-between">
                <span class="text-slate-400">IP Address:</span>
                <span class="text-white">{wifiStatus.ip}</span>
              </div>
            )}
          </div>
        </div>

        {/* WiFi Configuration Card */}
        <div class="bg-slate-800 rounded-2xl p-6 shadow-lg">
          <h3 class="text-lg font-semibold mb-4">WiFi Configuration</h3>

          {isLoading ? (
            <div class="flex items-center justify-center py-8">
              <Loader2 class="w-6 h-6 animate-spin text-slate-400" />
            </div>
          ) : (
            <div class="space-y-4">
              <div>
                <label
                  for="ssid"
                  class="block text-sm font-medium text-slate-300 mb-2"
                >
                  Network Name (SSID)
                </label>
                <input
                  id="ssid"
                  type="text"
                  value={wifiConfig.ssid}
                  onInput={(e) =>
                    setWifiConfig({
                      ...wifiConfig,
                      ssid: (e.target as HTMLInputElement).value,
                    })
                  }
                  placeholder="Enter WiFi network name"
                  class="w-full bg-slate-700 text-white px-4 py-3 rounded-lg border border-slate-600 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                />
              </div>

              <div>
                <label
                  for="password"
                  class="block text-sm font-medium text-slate-300 mb-2"
                >
                  Password
                </label>
                <input
                  id="password"
                  type="password"
                  value={wifiConfig.password}
                  onInput={(e) =>
                    setWifiConfig({
                      ...wifiConfig,
                      password: (e.target as HTMLInputElement).value,
                    })
                  }
                  placeholder="Enter WiFi password"
                  class="w-full bg-slate-700 text-white px-4 py-3 rounded-lg border border-slate-600 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                />
              </div>

              {saveMessage && (
                <div
                  class={`p-3 rounded-lg text-sm ${
                    saveMessage.type === "success"
                      ? "bg-green-900/50 text-green-300 border border-green-700"
                      : "bg-red-900/50 text-red-300 border border-red-700"
                  }`}
                >
                  {saveMessage.text}
                </div>
              )}

              <button
                onClick={saveWiFiConfig}
                disabled={isSaving}
                class="w-full bg-blue-600 text-white font-semibold py-3 px-6 rounded-lg transition-all duration-200 hover:bg-blue-700 active:scale-[0.98] disabled:opacity-50 disabled:cursor-not-allowed flex items-center justify-center gap-2"
              >
                {isSaving ? (
                  <>
                    <Loader2 class="w-4 h-4 animate-spin" />
                    Saving...
                  </>
                ) : (
                  <>
                    <Save class="w-4 h-4" />
                    Save WiFi Credentials
                  </>
                )}
              </button>

              <p class="text-xs text-slate-400 mt-2">
                After saving, restart the device to connect to the new network.
                If connection fails, the device will fall back to Access Point
                mode.
              </p>
            </div>
          )}
        </div>

        {/* Battery Calibration Card */}
        <div class="bg-slate-800 rounded-2xl p-6 shadow-lg">
          <h3 class="text-lg font-semibold mb-4 flex items-center gap-2">
            <Battery class="w-5 h-5" />
            Battery Calibration
          </h3>

          {isLoadingBattery ? (
            <div class="flex items-center justify-center py-8">
              <Loader2 class="w-6 h-6 animate-spin text-slate-400" />
            </div>
          ) : (
            <div class="space-y-4">
              <div>
                <label
                  for="batteryCalibrationFactor"
                  class="block text-sm font-medium text-slate-300 mb-2"
                >
                  Battery Calibration Factor
                </label>
                <input
                  id="batteryCalibrationFactor"
                  type="number"
                  step="0.001"
                  value={batteryCalibration.batteryCalibrationFactor}
                  onInput={(e) =>
                    setBatteryCalibration({
                      ...batteryCalibration,
                      batteryCalibrationFactor:
                        parseFloat((e.target as HTMLInputElement).value) || 1.0,
                    })
                  }
                  placeholder="1.0"
                  class="w-full bg-slate-700 text-white px-4 py-3 rounded-lg border border-slate-600 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                />
                <p class="text-xs text-slate-400 mt-1">
                  Multiplier to adjust battery voltage readings (default: 1.0)
                </p>
              </div>

              <div>
                <label
                  for="vbusCalibrationFactor"
                  class="block text-sm font-medium text-slate-300 mb-2"
                >
                  VBUS Calibration Factor
                </label>
                <input
                  id="vbusCalibrationFactor"
                  type="number"
                  step="0.001"
                  value={batteryCalibration.vbusCalibrationFactor}
                  onInput={(e) =>
                    setBatteryCalibration({
                      ...batteryCalibration,
                      vbusCalibrationFactor:
                        parseFloat((e.target as HTMLInputElement).value) || 1.0,
                    })
                  }
                  placeholder="1.0"
                  class="w-full bg-slate-700 text-white px-4 py-3 rounded-lg border border-slate-600 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                />
                <p class="text-xs text-slate-400 mt-1">
                  Multiplier to adjust USB voltage readings (default: 1.0)
                </p>
              </div>

              {batterySaveMessage && (
                <div
                  class={`p-3 rounded-lg text-sm ${
                    batterySaveMessage.type === "success"
                      ? "bg-green-900/50 text-green-300 border border-green-700"
                      : "bg-red-900/50 text-red-300 border border-red-700"
                  }`}
                >
                  {batterySaveMessage.text}
                </div>
              )}

              <button
                onClick={saveBatteryCalibration}
                disabled={isSavingBattery}
                class="w-full bg-blue-600 text-white font-semibold py-3 px-6 rounded-lg transition-all duration-200 hover:bg-blue-700 active:scale-[0.98] disabled:opacity-50 disabled:cursor-not-allowed flex items-center justify-center gap-2"
              >
                {isSavingBattery ? (
                  <>
                    <Loader2 class="w-4 h-4 animate-spin" />
                    Saving...
                  </>
                ) : (
                  <>
                    <Save class="w-4 h-4" />
                    Save Battery Calibration
                  </>
                )}
              </button>

              <p class="text-xs text-slate-400 mt-2">
                Adjust these values to calibrate voltage readings. Use a
                multimeter to measure actual voltages and adjust factors
                accordingly.
              </p>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
