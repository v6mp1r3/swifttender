import { useState, useEffect } from 'react'
import { useParams, useNavigate, Link } from 'react-router-dom'
import { api } from '../api/client'
import { useAuth } from '../context/AuthContext'
import ThresholdBadge from '../components/ThresholdBadge'

const fmtDate = ts => new Date(ts * 1000).toLocaleDateString('en-GB', {
  day: '2-digit', month: 'long', year: 'numeric', hour: '2-digit', minute: '2-digit'
})

const statusBadge = s => ({
  OPEN:       'badge-open',
  EVALUATION: 'badge-evaluation',
  AWARDED:    'badge-awarded',
  CANCELLED:  'badge-cancelled',
  DRAFT:      'badge-draft',
}[s] || 'badge-draft')

/* Required documents decoded from bitmask */
const DOC_FLAGS = [
  { bit: 1,  label: 'Company registration certificate' },
  { bit: 2,  label: 'VAT certificate' },
  { bit: 4,  label: 'Professional license' },
  { bit: 8,  label: 'Quality certificate (ISO)' },
  { bit: 16, label: 'Environmental certificate' },
  { bit: 32, label: 'Conflict of interest declaration' },
  { bit: 64, label: 'Technical specification compliance' },
]

export default function TenderDetail() {
  const { id }     = useParams()
  const { user }   = useAuth()
  const navigate   = useNavigate()

  const [tender,  setTender]  = useState(null)
  const [loading, setLoading] = useState(true)
  const [error,   setError]   = useState('')
  const [cancelling, setCancelling] = useState(false)

  useEffect(() => {
    api.get(`/api/tenders/${id}`)
      .then(d  => setTender(d.tender))
      .catch(e => setError(e.message))
      .finally(() => setLoading(false))
  }, [id])

  const handleCancel = async () => {
    if (!window.confirm('Cancel this tender? This cannot be undone.')) return
    setCancelling(true)
    try {
      await api.delete(`/api/tenders/${id}`)
      navigate('/dashboard')
    } catch (err) {
      setError(err.message)
    } finally {
      setCancelling(false)
    }
  }

  if (loading) return (
    <div className="text-center py-16 text-gray-400 text-sm">Loading…</div>
  )
  if (error)  return (
    <div className="card border-red-200 bg-red-50 text-red-700 text-sm">{error}</div>
  )
  if (!tender) return null

  const isOwner     = user?.id === tender.authorityId
  const isSupplier  = user?.role === 'SUPPLIER'
  const isOpen      = tender.status === 'OPEN'
  const pastDeadline= tender.deadline && Date.now() > tender.deadline * 1000
  const requiredDocs= DOC_FLAGS.filter(d => (tender.requiredDocs & d.bit) !== 0)

  return (
    <div className="max-w-3xl mx-auto">
      {/* Back */}
      <Link to="/tenders"
            className="inline-flex items-center gap-1.5 text-sm text-gray-500
                       hover:text-gray-800 mb-6 transition-colors">
        ← All tenders
      </Link>

      {/* Header card */}
      <div className="card mb-4">
        <div className="flex items-start justify-between gap-4 mb-4">
          <div>
            <div className="flex items-center gap-2 mb-2">
              <span className={statusBadge(tender.status)}>{tender.status}</span>
              <span className="text-xs text-gray-400">{tender.category}</span>
              {tender.cpvCode && (
                <span className="text-xs text-gray-400">CPV: {tender.cpvCode}</span>
              )}
            </div>
            <h1 className="text-xl font-semibold text-gray-900">{tender.title}</h1>
          </div>
          <div className="text-right shrink-0">
            <p className="text-2xl font-semibold text-gray-900">
              {tender.estimatedValue?.toLocaleString()}
            </p>
            <p className="text-xs text-gray-400">MDL excl. VAT</p>
          </div>
        </div>

        {tender.description && (
          <p className="text-sm text-gray-600 mb-4">{tender.description}</p>
        )}

        <ThresholdBadge
          category={tender.category}
          value={tender.estimatedValue}
        />
      </div>

      {/* Details grid */}
      <div className="grid grid-cols-2 gap-4 mb-4">
        <div className="card">
          <p className="text-xs text-gray-400 mb-1">Award criterion</p>
          <p className="text-sm font-medium text-gray-800">
            {{
              LOWEST_PRICE: 'Lowest price',
              LOWEST_COST:  'Lowest cost',
              BEST_QP:      'Best quality–price',
              BEST_QC:      'Best quality–cost',
            }[tender.awardCriterion] || tender.awardCriterion}
          </p>
          <p className="text-xs text-gray-400 mt-2">
            Price weight: {tender.priceWeight}% ·
            Delivery: {tender.deliveryWeight}%
          </p>
        </div>

        <div className="card">
          <p className="text-xs text-gray-400 mb-1">Submission deadline</p>
          <p className={`text-sm font-medium ${pastDeadline ? 'text-red-600' : 'text-gray-800'}`}>
            {tender.deadline ? fmtDate(tender.deadline) : 'Not set'}
          </p>
          {pastDeadline && (
            <p className="text-xs text-red-500 mt-1">Deadline has passed</p>
          )}
        </div>
      </div>

      {/* Required documents */}
      {requiredDocs.length > 0 && (
        <div className="card mb-4">
          <p className="text-sm font-medium text-gray-700 mb-2">
            Required documents from suppliers
          </p>
          <ul className="space-y-1">
            {requiredDocs.map(d => (
              <li key={d.bit} className="flex items-center gap-2 text-sm text-gray-600">
                <span className="text-green-500">✓</span> {d.label}
              </li>
            ))}
          </ul>
        </div>
      )}

      {/* Actions */}
      <div className="flex gap-3 flex-wrap">
        {/* Supplier: submit offer */}
        {isSupplier && isOpen && !pastDeadline && (
          <Link to={`/tenders/${id}/submit`} className="btn-primary">
            Submit offer
          </Link>
        )}

        {/* Authority: view offers */}
        {isOwner && (
          <Link to={`/tenders/${id}/offers`} className="btn-primary">
            View offers
          </Link>
        )}

        {/* Contract (if awarded) */}
        {tender.status === 'AWARDED' && user && (
          <Link to={`/tenders/${id}/contract`} className="btn-secondary">
            View contract
          </Link>
        )}

        {/* Authority: cancel */}
        {isOwner && isOpen && (
          <button
            onClick={handleCancel}
            disabled={cancelling}
            className="btn-danger ml-auto"
          >
            {cancelling ? 'Cancelling…' : 'Cancel tender'}
          </button>
        )}
      </div>

      {/* Legal note */}
      <p className="text-xs text-gray-400 mt-6">
        This tender is governed by Law 131/2015 on Public Procurement and
        Government Decision No. 870/2022 on low-value public procurement.
      </p>
    </div>
  )
}
