interface WeightCardProps {
  weight: number;
  isLoading: boolean;
}

export function WeightCard({ weight, isLoading }: WeightCardProps) {
  const formatWeight = (weight: number) => {
    if (weight < 0 && weight > -0.1) return "0.0";
    return weight.toFixed(1);
  };

  return (
    <div class="bg-linear-to-br from-slate-800 to-slate-900 rounded-3xl p-8 border border-slate-700/50 shadow-2xl">
      <div class="text-center">
        <div class="text-xs uppercase tracking-wider text-slate-400 mb-4 font-medium">
          Weight
        </div>
        {isLoading ? (
          <div class="h-32 flex items-center justify-center">
            <div class="w-8 h-8 border-2 border-slate-600 border-t-white rounded-full animate-spin"></div>
          </div>
        ) : (
          <>
            <div
              class="text-7xl font-light mb-2 tabular-nums"
              style="font-variant-numeric: tabular-nums;"
            >
              {formatWeight(weight)}
            </div>
            <div class="text-xl text-slate-400 font-light">grams</div>
          </>
        )}
      </div>
    </div>
  );
}

