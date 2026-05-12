import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { api } from '../api/client'
import ThresholdBadge from '../components/ThresholdBadge'

/* Document types that authority can require from suppliers.
 * Each maps to a bit in the uint8_t required_docs bitmask (tender.h). */
const DOC_FLAGS = [
  { bit: 1,  label: 'Company registration certificate' },
  { bit: 2,  label: 'VAT certificate' },
  { bit: 4,  label: 'Professional license / authorization' },
  { bit: 8,  label: 'Quality certificate (ISO or equivalent)' },
  { bit: 16, label: 'Environmental certificate' },
  { bit: 32, label: 'Conflict of interest declaration' },
  { bit: 64, label: 'Technical specification compliance document' },
]

const CATEGORIES = [
  { value: 'GOODS',  label: 'Goods & Services',  limit: '300,000 MDL' },
  { value: 'WORKS',  label: 'Works',              limit: '375,000 MDL' },
  { value: 'SOCIAL', label: 'Social Services',    limit: '600,000 MDL' },
]

const CRITERIA = [
  { value: 'LOWEST_PRICE', label: 'Lowest price' },
  { value: 'LOWEST_COST',  label: 'Lowest cost (lifecycle)' },
  { value: 'BEST_QP',      label: 'Best quality–price ratio' },
  { value: 'BEST_QC',      label: 'Best quality–cost ratio' },
]

export default function PostTender() {
  const navigate = useNavigate()

  const [form, setForm] = useState({
    title:          '',
    description:    '',
    cpvCode:        '',
    category:       'GOODS',
    estimatedValue: '',
    awardCriterion: 'LOWEST_PRICE',
    priceWeight:    60,
    deliveryWeight: 40,
    deadline:       '',
    requiredDocs:   0,
  })
  const [error,    setError]   = useState('')
  const [redirect, setRedirect]= useState(null) /* MTender redirect message */
  const [loading,  setLoading] = useState(false)

  const set = (key, val) => setForm(f => ({ ...f, [key]: val }))

  /* Toggle a document bit in the requiredDocs bitmask */
  const toggleDoc = bit =>
    setForm(f => ({ ...f, requiredDocs: f.requiredDocs ^ bit }))

  /* Keep price + delivery weights summing to 100 */
  const setPriceWeight = v => {
    const n = Math.max(0, Math.min(100, Number(v)))
    setForm(f => ({ ...f, priceWeight: n, deliveryWeight: 100 - n }))
  }

  const handleSubmit = async e => {
    e.preventDefault()
    setError(''); setRedirect(null)

    if (!form.title.trim()) { setError('Title is required'); return }
    if (Number(form.estimatedValue) <= 0) {
      setError('Estimated value must be greater than 0'); return
    }
    if (!form.deadline) { setError('Deadline is required'); return }

    const deadlineTs = Math.floor(new Date(form.deadline).getTime() / 1000)
    if (deadlineTs <= Date.now() / 1000) {
      setError('Deadline must be in the future'); return
    }

    setLoading(true)
    try {
      const data = await api.post('/api/tenders', {
        title:          form.title,
        description:    form.description,
        cpvCode:        form.cpvCode,
        category:       form.category,
        estimatedValue: Number(form.estimatedValue),
        awardCriterion: form.awardCriterion,
        priceWeight:    form.priceWeight,
        deliveryWeight: form.deliveryWeight,
        deadline:       deadlineTs,
        requiredDocs:   form.requiredDocs,
      })
      navigate(`/tenders/${data.tender.id}`)
    } catch (err) {
      /* 422 = threshold exceeded → redirect to MTender */
      if (err.message?.includes('MTender') || err.message?.includes('threshold')) {
        setRedirect(err.message)
      } else {
        setError(err.message || 'Failed to create tender')
      }
    } finally {
      setLoading(false)
    }
  }

  const value = Number(form.estimatedValue) || 0

  return (
    <div className="max-w-2xl mx-auto">
      <div className="mb-6">
        <h1 className="text-xl font-semibold text-gray-900">Post a tender</h1>
        <p className="text-sm text-gray-500 mt-0.5">
          Low-value procurement procedure · HG 870/2022
        </p>
      </div>

      {/* MTender redirect notice */}
      {redirect && (
        <div className="card border-amber-300 bg-amber-50 mb-4">
          <p className="text-sm font-medium text-amber-800 mb-1">
            ⚠ Value exceeds low-value threshold
          </p>
          <p className="text-sm text-amber-700">{redirect}</p>
          <a href="https://mtender.gov.md" target="_blank" rel="noreferrer"
             className="btn-primary mt-3 inline-flex text-sm">
            Go to MTender →
          </a>
        </div>
      )}

      {error && (
        <div className="mb-4 px-3 py-2 rounded-lg bg-red-50 border
                        border-red-200 text-sm text-red-700">
          {error}
        </div>
      )}

      <form onSubmit={handleSubmit} className="space-y-6">

        {/* Basic info */}
        <div className="card space-y-4">
          <h2 className="text-sm font-medium text-gray-700 border-b pb-2">
            Basic information
          </h2>

          <div>
            <label className="label">Title *</label>
            <input className="input" placeholder="e.g. Office supplies for Q3 2026"
              value={form.title}
              onChange={e => set('title', e.target.value)} required />
          </div>

          <div>
            <label className="label">Description</label>
            <textarea className="input resize-none" rows={3}
              placeholder="Additional details about the procurement..."
              value={form.description}
              onChange={e => set('description', e.target.value)} />
          </div>

          <div className="grid grid-cols-2 gap-4">
            <div>
              <label className="label">Category *</label>
              <select className="input" value={form.category}
                onChange={e => set('category', e.target.value)}>
                {CATEGORIES.map(c => (
                  <option key={c.value} value={c.value}>
                    {c.label} (≤ {c.limit})
                  </option>
                ))}
              </select>
            </div>
            <div>
              <label className="label">CPV code <span className="text-gray-400 font-normal">(optional)</span></label>
              <input className="input" placeholder="e.g. 30192000-1"
                value={form.cpvCode}
                onChange={e => set('cpvCode', e.target.value)} />
            </div>
          </div>

          <div className="grid grid-cols-2 gap-4">
            <div>
              <label className="label">Estimated value (MDL, excl. VAT) *</label>
              <input className="input" type="number" min="1" step="0.01"
                placeholder="e.g. 45000"
                value={form.estimatedValue}
                onChange={e => set('estimatedValue', e.target.value)} required />
            </div>
            <div>
              <label className="label">Submission deadline *</label>
              <input className="input" type="datetime-local"
                value={form.deadline}
                onChange={e => set('deadline', e.target.value)} required />
            </div>
          </div>

          {/* Live threshold badge */}
          {value > 0 && (
            <ThresholdBadge category={form.category} value={value} />
          )}
        </div>

        {/* Award criteria */}
        <div className="card space-y-4">
          <h2 className="text-sm font-medium text-gray-700 border-b pb-2">
            Evaluation criteria
          </h2>

          <div>
            <label className="label">Award criterion *</label>
            <select className="input" value={form.awardCriterion}
              onChange={e => set('awardCriterion', e.target.value)}>
              {CRITERIA.map(c => (
                <option key={c.value} value={c.value}>{c.label}</option>
              ))}
            </select>
          </div>

          <div>
            <label className="label">
              Scoring weights
              <span className="ml-2 text-xs font-normal text-gray-400">
                Price {form.priceWeight}% · Delivery {form.deliveryWeight}%
              </span>
            </label>
            <input type="range" min="0" max="100" step="5"
              className="w-full accent-blue-600"
              value={form.priceWeight}
              onChange={e => setPriceWeight(e.target.value)} />
            <div className="flex justify-between text-xs text-gray-400 mt-1">
              <span>Price focus</span>
              <span>Delivery focus</span>
            </div>
          </div>
        </div>

        {/* Required documents */}
        <div className="card space-y-3">
          <h2 className="text-sm font-medium text-gray-700 border-b pb-2">
            Required supplier documents
            <span className="ml-2 text-xs font-normal text-gray-400">
              (optional — check all that apply)
            </span>
          </h2>
          {DOC_FLAGS.map(d => (
            <label key={d.bit}
                   className="flex items-center gap-3 text-sm text-gray-700 cursor-pointer">
              <input type="checkbox"
                className="rounded border-gray-300 text-blue-600
                           focus:ring-blue-500 w-4 h-4"
                checked={(form.requiredDocs & d.bit) !== 0}
                onChange={() => toggleDoc(d.bit)} />
              {d.label}
            </label>
          ))}
        </div>

        {/* Submit */}
        <div className="flex gap-3">
          <button type="submit" className="btn-primary" disabled={loading}>
            {loading ? 'Publishing…' : 'Publish tender'}
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
