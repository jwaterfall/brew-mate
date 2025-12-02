interface UsbCardProps {
  connected: boolean;
}

export function UsbCard({ connected }: UsbCardProps) {
  return (
    <div class="bg-slate-800/50 backdrop-blur-sm rounded-2xl p-5 border border-slate-700/50">
      <div class="text-xs uppercase tracking-wider text-slate-400 mb-3 font-medium">
        USB
      </div>
      <div class="flex items-center gap-2 mb-1">
        <div
          class={`w-2.5 h-2.5 rounded-full shrink-0 ${
            connected ? "bg-green-400" : "bg-slate-600"
          }`}
        ></div>
        <span class="text-sm font-medium text-slate-200">
          {connected ? "Connected" : "Disconnected"}
        </span>
      </div>
      {connected && <div class="text-xs text-slate-400 mt-2">Charging</div>}
    </div>
  );
}
