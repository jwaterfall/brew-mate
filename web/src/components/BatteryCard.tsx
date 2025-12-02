import { Zap } from "lucide-preact";

interface BatteryCardProps {
  batteryPercent: number;
  batteryVoltage: number;
  usbConnected: boolean;
}

export function BatteryCard({
  batteryPercent,
  batteryVoltage,
  usbConnected,
}: BatteryCardProps) {
  const getBatteryColorClass = (percent: number) => {
    if (percent >= 50) return "text-green-500";
    if (percent >= 20) return "text-amber-500";
    return "text-red-500";
  };

  const getBatteryBgColorClass = (percent: number) => {
    if (percent >= 50) return "bg-green-500";
    if (percent >= 20) return "bg-amber-500";
    return "bg-red-500";
  };

  return (
    <div class="bg-slate-800/50 backdrop-blur-sm rounded-2xl p-5 border border-slate-700/50">
      <div class="flex items-center justify-between mb-3">
        <div class="text-xs uppercase tracking-wider text-slate-400 font-medium">
          Battery
        </div>
        {usbConnected && <Zap class="w-4 h-4 text-green-400" />}
      </div>
      <div
        class={`text-3xl font-semibold mb-1 tabular-nums ${getBatteryColorClass(
          batteryPercent
        )}`}
      >
        {batteryPercent}%
      </div>
      <div class="text-xs text-slate-400 mb-3 tabular-nums">
        {batteryVoltage.toFixed(2)}V
      </div>
      <div class="h-1.5 bg-slate-700 rounded-full overflow-hidden">
        <div
          class={`h-full rounded-full transition-all duration-500 ${getBatteryBgColorClass(
            batteryPercent
          )}`}
          style={`width: ${batteryPercent}%`}
        ></div>
      </div>
    </div>
  );
}
