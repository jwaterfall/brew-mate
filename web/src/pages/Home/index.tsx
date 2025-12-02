import { useState, useEffect } from "preact/hooks";
import { Loader2 } from "lucide-preact";

import { WeightCard } from "../../components/WeightCard";
import { BatteryCard } from "../../components/BatteryCard";
import { UsbCard } from "../../components/UsbCard";

interface ScaleStatus {
  weight: number;
  batteryPercent: number;
  batteryVoltage: number;
  usbConnected: boolean;
}

export function Home() {
  const [status, setStatus] = useState<ScaleStatus>({
    weight: 0,
    batteryPercent: 0,
    batteryVoltage: 0,
    usbConnected: false,
  });
  const [isLoading, setIsLoading] = useState(true);
  const [isTaring, setIsTaring] = useState(false);

  const fetchStatus = async () => {
    try {
      const response = await fetch("/api/status");
      if (!response.ok) {
        throw new Error("Service unavailable");
      }
      const data = await response.json();
      setStatus(data);
      setIsLoading(false);
    } catch (error) {
      console.error("Failed to fetch status:", error);
      setIsLoading(false);
    }
  };

  const handleTare = async () => {
    setIsTaring(true);
    try {
      const response = await fetch("/api/tare", {
        method: "POST",
      });
      const result = await response.json();
      if (result.success) {
        setTimeout(() => {
          fetchStatus();
          setIsTaring(false);
        }, 500);
      } else {
        setIsTaring(false);
      }
    } catch (error) {
      console.error("Failed to tare:", error);
      setIsTaring(false);
    }
  };

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 500);
    return () => clearInterval(interval);
  }, []);

  return (
    <div class="min-h-screen">
      <div class="max-w-lg mx-auto py-4 px-4 sm:px-6 lg:px-8 space-y-4">
        <WeightCard weight={status.weight} isLoading={isLoading} />

        <div class="grid grid-cols-2 gap-4">
          <BatteryCard
            batteryPercent={status.batteryPercent}
            batteryVoltage={status.batteryVoltage}
            usbConnected={status.usbConnected}
          />
          <UsbCard usbConnected={status.usbConnected} />
        </div>

        <button
          onClick={handleTare}
          disabled={isTaring}
          class="w-full bg-white text-slate-900 font-semibold py-4 px-6 rounded-2xl transition-all duration-200 hover:bg-slate-100 active:scale-[0.98] disabled:opacity-50 disabled:cursor-not-allowed shadow-lg hover:shadow-xl"
        >
          {isTaring ? (
            <span class="flex items-center justify-center gap-2">
              <Loader2 class="w-4 h-4 animate-spin" />
              Taring...
            </span>
          ) : (
            "Tare Scale"
          )}
        </button>
      </div>
    </div>
  );
}
