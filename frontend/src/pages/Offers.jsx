import { useState, useEffect } from 'react'
import { useParams, useNavigate, Link } from 'react-router-dom'
import { api } from '../api/client'
import RankingTable from '../components/RankingTable'

export default function Offers() {
  const { id }   = useParams()
  const navigate = useNavigate()

  const [tender,    setTender]    = useState(null)
  const [offers,    setOffers]    = useState([])
  const [loading,   setLoading]   = useState(true)
  const [error,     setError]     = useState('')
  const [selecting, setSelecting] = useState(false)
  const [confirm,   setConfirm]   = useState(null)  /* offer pending confirmation */

  const load = async () => {
    setLoading(true)
    try {
      const [tRes, oRes] = await Promise.all([
        api.get(`/api/tenders/${id}`),
        api.get(`/api/tenders/${id}/offers`),
      ])
      setTender(tRes.tender)
      setOffers(oRes.offers || [])
    } catch (err) {
      setError(err.message)
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => { load() }, [id])

  /* Called when authority clicks "Select" on an offer */
  const handleSelectWinner = offer => {
    setConfirm(offer)   /* show confirmation dialog */
  }

  /* Confirmed — call the API */
  const handleConfirm = async () => {
    if (!confirm) return
    setSelecting(true)
    try {
      await api.post(`/api/tenders/${id}/winner`, { offerId: confirm.id })
      await load()   /* refresh — offers now show WINNER/LOSER statuses */
      setConfirm(null)
    } catch (err) {
      setError(err.message)
      setConfirm(null)
    } finally {
      setSelecting(false)
    }
  }

  const isAwarded = tender?.status === 'AWARDED'
  const canSelect = !isAwarded && offers.some(
    o => o.status !== 'DISQUALIFIED'
  )

  if (loading) return (
    <div className="text-center py-16 text-gray-400 text-sm">Loading offers…</div>
  )

  return (
    <div className="max-w-5xl mx-auto">

      {/* Back */}
      <Link to={`/tenders/${id}`}
            className="inline-flex items-center gap-1.5 text-sm text-gray-500
                       hover:text-gray-800 mb-6 transition-colors">
        ← Tender detail
      </Link>

      {/* Header */}
      <div className="flex items-start justify-between mb-6">
        <div>
          <h1 className="text-xl font-semibold text-gray-900">Offers</h1>
          {tender && (
            <p className="text-sm text-gray-500 mt-0.5">
              {tender.title} · {offers.length} offer{offers.length !== 1 ? 's' : ''}
            </p>
          )}
        </div>
        {isAwarded && (
          <Link to={`/tenders/${id}/contract`} className="btn-primary">
            View contract →
          </Link>
        )}
      </div>

      {error && (
        <div className="mb-4 px-3 py-2 rounded-lg bg-red-50 border
                        border-red-200 text-sm text-red-700">
          {error}
        </div>
      )}

      {/* Scoring explanation */}
      {offers.length > 0 && (
        <div className="card mb-5 bg-blue-50 border-blue-200">
          <p className="text-xs text-blue-700">
            <strong>How ranking works:</strong> Offers are sorted by the
            server-side min-heap algorithm. Each offer gets a weighted score:
            {' '}<code className="bg-blue-100 px-1 rounded">
              score = {tender?.priceWeight || 60}% × (price / min_price)
              + {tender?.deliveryWeight || 40}% × (days / min_days)
            </code>.
            Lower score = better offer. Rank 1 is the economically most
            advantageous offer (HG 870/2022, Art. 25).
          </p>
        </div>
      )}

      {/* Ranked table (DSA: results from C min-heap on server) */}
      <RankingTable
        offers={offers}
        canSelect={canSelect}
        onSelectWinner={handleSelectWinner}
      />

      {/* Awarded notice */}
      {isAwarded && (
        <div className="mt-4 card border-green-200 bg-green-50">
          <p className="text-sm font-medium text-green-800">
            ✓ Winner selected — tender awarded.
          </p>
          <p className="text-xs text-green-700 mt-0.5">
            The winning supplier has been notified. The contract is ready for
            electronic signing via MSign.
          </p>
        </div>
      )}

      {/* Confirmation dialog */}
      {confirm && (
        <div className="fixed inset-0 bg-black/40 flex items-center justify-center z-50 p-4">
          <div className="bg-white rounded-2xl shadow-xl max-w-sm w-full p-6">
            <h3 className="font-semibold text-gray-900 mb-2">
              Confirm winner selection
            </h3>
            <p className="text-sm text-gray-600 mb-1">
              You are selecting offer <strong>#{confirm.id}</strong> as the winner:
            </p>
            <div className="rounded-lg bg-gray-50 border p-3 text-sm mb-4">
              <div className="flex justify-between">
                <span className="text-gray-500">Price</span>
                <span className="font-medium">{confirm.price?.toLocaleString()} MDL</span>
              </div>
              <div className="flex justify-between mt-1">
                <span className="text-gray-500">Delivery</span>
                <span className="font-medium">{confirm.deliveryDays} days</span>
              </div>
              <div className="flex justify-between mt-1">
                <span className="text-gray-500">Rank</span>
                <span className="font-medium">
                  {confirm.rank === 1 ? '🥇 #1 (recommended)' : `#${confirm.rank}`}
                </span>
              </div>
            </div>
            {confirm.rank !== 1 && (
              <div className="rounded-lg bg-amber-50 border border-amber-200
                              p-3 text-xs text-amber-700 mb-4">
                ⚠ You are not selecting the top-ranked offer. This action will
                be recorded in the audit log as an override.
              </div>
            )}
            <div className="flex gap-3">
              <button
                onClick={handleConfirm}
                disabled={selecting}
                className="btn-primary flex-1"
              >
                {selecting ? 'Confirming…' : 'Confirm winner'}
              </button>
              <button
                onClick={() => setConfirm(null)}
                className="btn-secondary flex-1"
              >
                Cancel
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}
