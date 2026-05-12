import { useState, useEffect } from 'react'
import { Link } from 'react-router-dom'
import { api } from '../api/client'
import { useAuth } from '../context/AuthContext'

/* Status badge colours matching index.css .badge-* classes */
const statusBadge = s => ({
  OPEN:       'badge-open',
  EVALUATION: 'badge-evaluation',
  AWARDED:    'badge-awarded',
  CANCELLED:  'badge-cancelled',
  DRAFT:      'badge-draft',
}[s] || 'badge-draft')

/* Format Unix timestamp to human-readable date */
const fmtDate = ts => new Date(ts * 1000).toLocaleDateString('en-GB', {
  day: '2-digit', month: 'short', year: 'numeric'
})

/* Deadline urgency: returns colour class based on days remaining */
const deadlineColor = ts => {
  const days = Math.ceil((ts * 1000 - Date.now()) / 86400000)
  if (days < 0)  return 'text-red-500'
  if (days <= 2) return 'text-orange-500'
  return 'text-gray-500'
}

export default function TenderList() {
  const { user } = useAuth()

  const [tenders,  setTenders]  = useState([])
  const [loading,  setLoading]  = useState(true)
  const [error,    setError]    = useState('')
  const [filters,  setFilters]  = useState({ status: '', category: '' })

  const load = async () => {
    setLoading(true)
    try {
      const params = new URLSearchParams()
      if (filters.status)   params.set('status',   filters.status)
      if (filters.category) params.set('category', filters.category)
      const data = await api.get(`/api/tenders?${params}`)
      setTenders(data.tenders || [])
    } catch (err) {
      setError(err.message)
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => { load() }, [filters])

  return (
    <div>
      {/* Page header */}
      <div className="flex items-center justify-between mb-6">
        <div>
          <h1 className="text-xl font-semibold text-gray-900">Active Tenders</h1>
          <p className="text-sm text-gray-500 mt-0.5">
            Low-value public procurement · Republic of Moldova
          </p>
        </div>
        {user?.role === 'AUTHORITY' && (
          <Link to="/tenders/new" className="btn-primary">
            + Post tender
          </Link>
        )}
      </div>

      {/* Filters */}
      <div className="flex flex-wrap gap-3 mb-6">
        <select
          className="input w-auto text-sm"
          value={filters.status}
          onChange={e => setFilters(f => ({ ...f, status: e.target.value }))}
        >
          <option value="">All statuses</option>
          <option value="OPEN">Open</option>
          <option value="EVALUATION">Under evaluation</option>
          <option value="AWARDED">Awarded</option>
        </select>

        <select
          className="input w-auto text-sm"
          value={filters.category}
          onChange={e => setFilters(f => ({ ...f, category: e.target.value }))}
        >
          <option value="">All categories</option>
          <option value="GOODS">Goods & Services</option>
          <option value="WORKS">Works</option>
          <option value="SOCIAL">Social Services</option>
        </select>

        {(filters.status || filters.category) && (
          <button
            onClick={() => setFilters({ status: '', category: '' })}
            className="btn-secondary text-sm"
          >
            Clear filters
          </button>
        )}
      </div>

      {/* States */}
      {loading && (
        <div className="text-center py-16 text-gray-400 text-sm">
          Loading tenders…
        </div>
      )}

      {error && (
        <div className="card border-red-200 bg-red-50 text-red-700 text-sm">
          {error}
        </div>
      )}

      {!loading && !error && tenders.length === 0 && (
        <div className="card text-center py-16">
          <p className="text-3xl mb-3">📋</p>
          <p className="text-gray-500 text-sm">No tenders found.</p>
          {user?.role === 'AUTHORITY' && (
            <Link to="/tenders/new" className="btn-primary mt-4 inline-flex">
              Post the first tender
            </Link>
          )}
        </div>
      )}

      {/* Tender grid */}
      <div className="grid gap-4">
        {tenders.map(t => (
          <Link key={t.id} to={`/tenders/${t.id}`}
                className="card hover:border-blue-300 hover:shadow-md
                           transition-all duration-150 group block">
            <div className="flex items-start justify-between gap-4">
              <div className="flex-1 min-w-0">
                <div className="flex items-center gap-2 mb-1">
                  <span className={statusBadge(t.status)}>{t.status}</span>
                  <span className="text-xs text-gray-400">{t.category}</span>
                </div>
                <h2 className="font-medium text-gray-900 group-hover:text-blue-700
                               truncate transition-colors">
                  {t.title}
                </h2>
                <p className="text-sm text-gray-500 mt-0.5 line-clamp-1">
                  {t.description || 'No description provided.'}
                </p>
              </div>

              <div className="text-right shrink-0">
                <p className="font-semibold text-gray-900 text-sm">
                  {t.estimatedValue?.toLocaleString()} MDL
                </p>
                <p className={`text-xs mt-0.5 ${deadlineColor(t.deadline)}`}>
                  {t.deadline
                    ? (Date.now() > t.deadline * 1000
                        ? 'Deadline passed'
                        : `Deadline: ${fmtDate(t.deadline)}`)
                    : '—'}
                </p>
              </div>
            </div>
          </Link>
        ))}
      </div>
    </div>
  )
}
