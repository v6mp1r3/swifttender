/*
 * ThresholdBadge.jsx
 * Shows which low-value procurement tier a tender falls into.
 * Tiers defined by HG 870/2022 (Moldova) — matches backend threshold.c
 */
const TIERS = {
  GOODS:  { label: 'Goods & Services', limit: 300000 },
  WORKS:  { label: 'Works',            limit: 375000 },
  SOCIAL: { label: 'Social Services',  limit: 600000 },
}

export default function ThresholdBadge({ category, value }) {
  const tier  = TIERS[category] || TIERS.GOODS
  const pct   = Math.min(100, Math.round((value / tier.limit) * 100))
  const color = pct < 50 ? 'bg-green-500' : pct < 85 ? 'bg-yellow-500' : 'bg-red-500'

  return (
    <div className="rounded-lg border border-gray-200 bg-gray-50 p-3 text-xs">
      <div className="flex justify-between text-gray-500 mb-1.5">
        <span>{tier.label} threshold</span>
        <span className="font-medium text-gray-700">
          {value.toLocaleString()} / {tier.limit.toLocaleString()} MDL
        </span>
      </div>
      <div className="h-1.5 rounded-full bg-gray-200 overflow-hidden">
        <div className={`h-full rounded-full ${color} transition-all`}
             style={{ width: `${pct}%` }} />
      </div>
      <p className="text-gray-400 mt-1">{pct}% of low-value limit (HG 870/2022)</p>
    </div>
  )
}
