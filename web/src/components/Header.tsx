import { Settings } from "lucide-preact";

export function Header() {
  return (
    <header class="bg-slate-900 border-b border-slate-800">
      <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div class="flex items-center justify-between h-18">
          <a href="/" class="hover:opacity-80 transition-opacity">
            <h1 class="text-2xl font-bold font-serif">BrewMate</h1>
            <p class="text-slate-400 text-sm">Precision scale for brewing</p>
          </a>
          <nav class="flex items-center gap-4">
            <a
              href="/settings"
              class="p-2 text-slate-400 hover:text-white hover:bg-slate-800 rounded-md transition-colors"
            >
              <Settings class="w-6 h-6" />
            </a>
          </nav>
        </div>
      </div>
    </header>
  );
}
