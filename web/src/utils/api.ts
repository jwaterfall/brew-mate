/**
 * Build API URL from path
 * Uses VITE_API_URL from .env for local dev, or relative paths when deployed
 * Examples: VITE_API_URL=http://10.69.2.19 or VITE_API_URL=10.69.2.19
 */
export function apiUrl(path: string): string {
  const base = import.meta.env.VITE_API_URL;
  const normalizedPath = path.startsWith("/") ? path : `/${path}`;

  if (!base) return normalizedPath;

  const baseUrl = base
    .replace(/\/$/, "")
    .replace(/^(?!https?:\/\/)/, "http://");
  return `${baseUrl}${normalizedPath}`;
}
