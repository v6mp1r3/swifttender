/*
 * RankingTable.jsx
 *
 * Displays offers sorted by weighted score (from the backend min-heap).
 * Rank 1 = best offer (lowest key from the C min-heap).
 *
 * Visual design:
 *   - Gold medal for rank 1, silver for rank 2, bronze for rank 3
 *   - Score bar: visual representation of relative score
 *   - Lowest score = longest bar (best offer fills 100%)
 */

const MEDALS = { 1: '🥇', 2: '🥈', 3: '🥉' }

const statusBadge = s => ({
  SUBMITTED:    'badge-draft',
  VALID:        'badge-open',
  DISQUALIFIED: 'badge-cancelled',
  WINNER:       'badge-awarded',
  LOSER:        'badge-draft',
}[s] || 'badge-draft')

export default function RankingTable({ offers, onSelectWinner, canSelect }) {
  if (!offers || offers.length === 0) {
    return (
      <div className="card text-center py-12">
        <p className="text-gray-400 text-sm">No offers submitted yet.</p>
      </div>
    )
  }

  /* Score bar width: rank 1 gets 100%, others scaled proportionally.
   * Since lower score = better, we invert: bar = min_score / offer_score */
  const minScore = offers[0]?.score || 1
  const barWidth = s => Math.round((minScore / s) * 100)

  return (
    <div className="overflow-hidden rounded-xl border border-gray-200">
      <table className="w-full text-sm">
        <thead className="bg-gray-50 border-b border-gray-200">
          <tr>
            <th className="text-left px-4 py-3 text-xs font-medium text-gray-500 w-16">Rank</th>
            <th className="text-left px-4 py-3 text-xs font-medium text-gray-500">Supplier</th>
            <th className="text-right px-4 py-3 text-xs font-medium text-gray-500">Price (MDL)</th>
            <th className="text-right px-4 py-3 text-xs font-medium text-gray-500">Delivery</th>
            <th className="text-left px-4 py-3 text-xs font-medium text-gray-500 w-48">Score</th>
            <th className="text-left px-4 py-3 text-xs font-medium text-gray-500">Status</th>
            {canSelect && <th className="px-4 py-3 w-28" />}
          </tr>
        </thead>
        <tbody className="divide-y divide-gray-100">
          {offers.map(offer => {
            const isTop      = offer.rank === 1
            const isWinner   = offer.status === 'WINNER'
            const rowClass   = isWinner
              ? 'bg-green-50'
              : isTop
              ? 'bg-blue-50/40'
              : 'bg-white hover:bg-gray-50'

            return (
              <tr key={offer.id} className={`${rowClass} transition-colors`}>
                {/* Rank */}
                <td className="px-4 py-3 font-medium text-gray-700">
                  <span className="text-lg">
                    {MEDALS[offer.rank] || `#${offer.rank}`}
                  </span>
                </td>

                {/* Supplier */}
                <td className="px-4 py-3">
                  <p className="font-medium text-gray-900">
                    Supplier #{offer.supplierId}
                  </p>
                  {offer.notes && (
                    <p className="text-xs text-gray-400 mt-0.5 truncate max-w-xs">
                      {offer.notes}
                    </p>
                  )}
                </td>

                {/* Price */}
                <td className="px-4 py-3 text-right font-medium text-gray-900">
                  {offer.price?.toLocaleString()}
                </td>

                {/* Delivery */}
                <td className="px-4 py-3 text-right text-gray-600">
                  {offer.deliveryDays}d
                </td>

                {/* Score bar */}
                <td className="px-4 py-3">
                  <div className="flex items-center gap-2">
                    <div className="flex-1 h-2 rounded-full bg-gray-200 overflow-hidden">
                      <div
                        className={`h-full rounded-full transition-all ${
                          isTop ? 'bg-blue-500' : 'bg-gray-400'
                        }`}
                        style={{ width: `${barWidth(offer.score)}%` }}
                      />
                    </div>
                    <span className="text-xs text-gray-500 w-10 text-right">
                      {offer.score?.toFixed(3)}
                    </span>
                  </div>
                </td>

                {/* Status */}
                <td className="px-4 py-3">
                  <span className={statusBadge(offer.status)}>
                    {offer.status}
                  </span>
                </td>

                {/* Action */}
                {canSelect && (
                  <td className="px-4 py-3 text-right">
                    {offer.status !== 'DISQUALIFIED' &&
                     offer.status !== 'WINNER' &&
                     offer.status !== 'LOSER' && (
                      <button
                        onClick={() => onSelectWinner(offer)}
                        className={`text-xs font-medium px-3 py-1.5 rounded-lg
                          transition-colors border
                          ${isTop
                            ? 'bg-blue-600 text-white border-blue-600 hover:bg-blue-700'
                            : 'border-gray-300 text-gray-600 hover:bg-gray-50'
                          }`}
                      >
                        {isTop ? '★ Select' : 'Select'}
                      </button>
                    )}
                    {isWinner && (
                      <span className="text-xs text-green-600 font-medium">
                        ✓ Winner
                      </span>
                    )}
                  </td>
                )}
              </tr>
            )
          })}
        </tbody>
      </table>
    </div>
  )
}
