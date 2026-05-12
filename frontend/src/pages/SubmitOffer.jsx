import { useState, useEffect } from 'react'
import { useParams, useNavigate } from 'react-router-dom'
import { api } from '../api/client'

/* Document types matching the backend bitmask */
const DOC_FLAGS = [
  { bit: 1,  key: 'reg',     label: 'Company registration certificate' },
  { bit: 2,  key: 'vat',     label: 'VAT certificate' },
  { bit: 4,  key: 'prof',    label: 'Professional license' },
  { bit: 8,  key: 'quality', label: 'Quality certificate' },
  { bit: 16, key: 'env',     label: 'Environmental certificate' },
  { bit: 32, key: 'conflict',label: 'Conflict of interest declaration' },
  { bit: 64, key: 'tech',    label: 'Technical specification compliance' },
]

const fmtDate = ts => new Date(ts * 1000).toLocaleDateString('en-GB', {
  day: '2-digit', month: 'short', year: 'numeric'
})

export default function SubmitOffer() {
  const { id }   = useParams()
  const navigate = useNavigate()

  const [tender,   setTender]   = useState(null)
  const [loading,  setLoading]  = useState(true)
  const [error,    setError]    = useState('')
  const [submitting, setSubmitting] = useState(false)
  const [success,  setSuccess]  = useState(false)

  const [form, setForm] = useState({
    price:        '',
    deliveryDays: '',
    notes:        '',
  })

  /* Track upload paths per document type */
  const [uploads, setUploads]   = useState({})
  const [uploading, setUploading] = useState({})

  useEffect(() => {
    api.get(`/api/tenders/${id}`)
      .then(d => setTender(d.tender))
      .catch(e => setError(e.message))
      .finally(() => setLoading(false))
  }, [id])

  /* Upload a single file and store its server path */
  const handleFileUpload = async (key, file) => {
    if (!file) return
    setUploading(u => ({ ...u, [key]: true }))
    try {
      const fd = new FormData()
      fd.append('file', file)
      const data = await api.upload('/api/uploads', fd)
      setUploads(u => ({ ...u, [key]: data.path }))
    } catch (err) {
      setError(`Upload failed for ${key}: ${err.message}`)
    } finally {
      setUploading(u => ({ ...u, [key]: false }))
    }
  }

  const handleSubmit = async e => {
    e.preventDefault()
    setError('')

    if (Number(form.price) <= 0) {
      setError('Price must be greater than 0'); return
    }
    if (Number(form.deliveryDays) <= 0) {
      setError('Delivery days must be greater than 0'); return
    }

    /* Check all required docs are uploaded */
    const required = DOC_FLAGS.filter(
      d => (tender.requiredDocs & d.bit) !== 0
    )
    const missing = required.filter(d => !uploads[d.key])
    if (missing.length > 0) {
      setError(`Please upload: ${missing.map(d => d.label).join(', ')}`)
      return
    }

    setSubmitting(true)
    try {
      await api.post(`/api/tenders/${id}/offers`, {
        price:        Number(form.price),
        deliveryDays: Number(form.deliveryDays),
        notes:        form.notes,
        docsPath:     Object.values(uploads).join(','),
      })
      setSuccess(true)
    } catch (err) {
      setError(err.message)
    } finally {
      setSubmitting(false)
    }
  }

  if (loading) return (
    <div className="text-center py-16 text-gray-400 text-sm">Loading…</div>
  )
  if (!tender) return (
    <div className="card text-red-700 text-sm">{error || 'Tender not found'}</div>
  )

  /* Success screen */
  if (success) return (
    <div className="max-w-md mx-auto mt-16 text-center">
      <div className="card">
        <p className="text-4xl mb-3">✅</p>
        <h2 className="text-lg font-semibold text-gray-900 mb-1">
          Offer submitted!
        </h2>
        <p className="text-sm text-gray-500 mb-4">
          Your offer for <strong>{tender.title}</strong> has been received.
          You will be notified when a winner is selected.
        </p>
        <button onClick={() => navigate('/dashboard')} className="btn-primary w-full">
          Back to dashboard
        </button>
      </div>
    </div>
  )

  const requiredDocs = DOC_FLAGS.filter(d => (tender.requiredDocs & d.bit) !== 0)

  return (
    <div className="max-w-xl mx-auto">
      <div className="mb-6">
        <h1 className="text-xl font-semibold text-gray-900">Submit offer</h1>
        <p className="text-sm text-gray-500 mt-0.5">
          Responding to: <strong>{tender.title}</strong>
        </p>
      </div>

      {/* Tender summary */}
      <div className="card mb-4 bg-blue-50 border-blue-200">
        <div className="flex justify-between text-sm">
          <div>
            <p className="text-xs text-blue-500 mb-0.5">Estimated budget</p>
            <p className="font-medium text-blue-900">
              {tender.estimatedValue?.toLocaleString()} MDL
            </p>
          </div>
          <div className="text-right">
            <p className="text-xs text-blue-500 mb-0.5">Deadline</p>
            <p className="font-medium text-blue-900">
              {fmtDate(tender.deadline)}
            </p>
          </div>
          <div className="text-right">
            <p className="text-xs text-blue-500 mb-0.5">Scoring</p>
            <p className="font-medium text-blue-900">
              Price {tender.priceWeight}% · Delivery {tender.deliveryWeight}%
            </p>
          </div>
        </div>
      </div>

      {error && (
        <div className="mb-4 px-3 py-2 rounded-lg bg-red-50 border
                        border-red-200 text-sm text-red-700">
          {error}
        </div>
      )}

      <form onSubmit={handleSubmit} className="space-y-5">

        {/* Pricing */}
        <div className="card space-y-4">
          <h2 className="text-sm font-medium text-gray-700 border-b pb-2">
            Your offer
          </h2>

          <div className="grid grid-cols-2 gap-4">
            <div>
              <label className="label">Price (MDL, excl. VAT) *</label>
              <input className="input" type="number" min="1" step="0.01"
                placeholder="e.g. 42000"
                value={form.price}
                onChange={e => setForm(f => ({ ...f, price: e.target.value }))}
                required />
              {form.price && Number(form.price) > tender.estimatedValue && (
                <p className="text-xs text-amber-600 mt-1">
                  Above estimated budget — your offer may rank lower.
                </p>
              )}
            </div>
            <div>
              <label className="label">Delivery time (days) *</label>
              <input className="input" type="number" min="1"
                placeholder="e.g. 5"
                value={form.deliveryDays}
                onChange={e => setForm(f => ({ ...f, deliveryDays: e.target.value }))}
                required />
            </div>
          </div>

          {/* Live score preview */}
          {form.price && form.deliveryDays && (
            <div className="rounded-lg bg-gray-50 border border-gray-200 p-3 text-xs">
              <p className="text-gray-500 mb-1">Scoring preview (vs your offer only)</p>
              <p className="text-gray-700">
                Price weight: <strong>{tender.priceWeight}%</strong> ·
                Delivery weight: <strong>{tender.deliveryWeight}%</strong>
              </p>
              <p className="text-gray-400 mt-1">
                Final rank is computed by the server using the min-heap
                ranking algorithm once all offers are submitted.
              </p>
            </div>
          )}

          <div>
            <label className="label">Notes <span className="text-gray-400 font-normal">(optional)</span></label>
            <textarea className="input resize-none" rows={2}
              placeholder="Any additional information..."
              value={form.notes}
              onChange={e => setForm(f => ({ ...f, notes: e.target.value }))} />
          </div>
        </div>

        {/* Document uploads */}
        {requiredDocs.length > 0 && (
          <div className="card space-y-4">
            <h2 className="text-sm font-medium text-gray-700 border-b pb-2">
              Required documents
            </h2>
            {requiredDocs.map(d => (
              <div key={d.bit}>
                <label className="label">
                  {d.label} *
                  {uploads[d.key] && (
                    <span className="ml-2 text-xs text-green-600 font-normal">
                      ✓ Uploaded
                    </span>
                  )}
                </label>
                <input type="file" accept=".pdf,.doc,.docx,.jpg,.png"
                  className="block w-full text-sm text-gray-500
                             file:mr-3 file:py-1.5 file:px-3 file:rounded-lg
                             file:border file:border-gray-300 file:text-sm
                             file:bg-white file:text-gray-700
                             hover:file:bg-gray-50"
                  disabled={uploading[d.key]}
                  onChange={e => handleFileUpload(d.key, e.target.files[0])} />
                {uploading[d.key] && (
                  <p className="text-xs text-blue-500 mt-1">Uploading…</p>
                )}
              </div>
            ))}
          </div>
        )}

        {/* MSign notice */}
        <div className="rounded-lg bg-amber-50 border border-amber-200 p-3 text-xs text-amber-700">
          <strong>Note:</strong> By submitting this offer you confirm that all
          information is accurate. The contract, if awarded, will require
          electronic signing via MSign.
        </div>

        <div className="flex gap-3">
          <button type="submit" className="btn-primary" disabled={submitting}>
            {submitting ? 'Submitting…' : 'Submit offer'}
          </button>
          <button type="button" className="btn-secondary"
            onClick={() => navigate(-1)}>
            Cancel
          </button>
        </div>
      </form>
    </div>
  )
}
