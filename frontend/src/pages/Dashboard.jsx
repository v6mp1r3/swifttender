import { useState, useEffect } from 'react'
import { Link } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'
import { api } from '../api/client'
import Notification from '../components/Notification'

const fmtDate = ts => new Date(ts * 1000).toLocaleDateString('en-GB', {
  day: '2-digit', month: 'short', year: 'numeric'
})

const statusBadge = s => ({
  OPEN:       'badge-open',
  EVALUATION: 'badge-evaluation',
  AWARDED:    'badge-awarded',
  CANCELLED:  'badge-cancelled',
  DRAFT:      'badge-draft',
}[s] || 'badge-draft')

/* ── Stat card ─────────────────────────────────────────────────── */
function StatCard({ label, value, sub, color = 'text-gray-900' }) {
  return (
    <div className="card text-center">
      <p className={`text-3xl font-semibold ${color}`}>{value}</p>
      <p className="text-xs text-gray-500 mt-1">{label}</p>
      {sub && <p className="text-xs text-gray-400 mt-0.5">{sub}</p>}
    </div>
  )
}

/* ── Authority dashboard ────────────────────────────────────────── */
function AuthorityDashboard({ user }) {
  const [tenders, setTenders] = useState([])
  const [notifs,  setNotifs]  = useState([])
  const [loading, setLoading] = useState(true)
  const [reportLoading, setReportLoading] = useState(false)
  const [report, setReport] = useState(null)

  useEffect(() => {
    Promise.all([
      api.get('/api/tenders'),
      api.get('/api/notifications'),
    ]).then(([td, nd]) => {
      /* Filter tenders belonging to this authority */
      setTenders((td.tenders || []).filter(t => t.authorityId === user.id))
      setNotifs(nd.notifications || [])
    }).catch(console.error)
      .finally(() => setLoading(false))
  }, [user.id])

  const open     = tenders.filter(t => t.status === 'OPEN').length
  const awarded  = tenders.filter(t => t.status === 'AWARDED').length
  const totalVal = tenders.reduce((s, t) => s + (t.estimatedValue || 0), 0)

  const generateReport = async () => {
    setReportLoading(true)
    const now = new Date()
    const q   = Math.ceil((now.getMonth() + 1) / 3)
    try {
      const data = await api.get(
        `/api/reports/quarterly?year=${now.getFullYear()}&quarter=${q}`
      )
      setReport(data)
    } catch (err) {
      console.error(err)
    } finally {
      setReportLoading(false)
    }
  }

  if (loading) return (
    <div className="text-center py-16 text-gray-400 text-sm">Loading…</div>
  )

  return (
    <div>
      {/* Welcome */}
      <div className="flex items-center justify-between mb-6">
        <div>
          <h1 className="text-xl font-semibold text-gray-900">
            Dashboard — {user.name}
          </h1>
          <p className="text-sm text-gray-500 mt-0.5">
            Contracting authority · IDNO {user.idno}
          </p>
        </div>
        <Link to="/tenders/new" className="btn-primary">+ Post tender</Link>
      </div>

      {/* Stats */}
      <div className="grid grid-cols-4 gap-4 mb-6">
        <StatCard label="Total tenders"  value={tenders.length} />
        <StatCard label="Open"  value={open}    color="text-green-600" />
        <StatCard label="Awarded" value={awarded} color="text-blue-600" />
        <StatCard label="Total value" value={`${(totalVal/1000).toFixed(0)}k MDL`} />
      </div>

      <div className="grid grid-cols-3 gap-6">
        {/* Tender list */}
        <div className="col-span-2">
          <div className="flex items-center justify-between mb-3">
            <h2 className="text-sm font-medium text-gray-700">Your tenders</h2>
            <Link to="/tenders" className="text-xs text-blue-600 hover:underline">
              Browse all →
            </Link>
          </div>
          <div className="space-y-2">
            {tenders.length === 0 && (
              <div className="card text-center py-8">
                <p className="text-gray-400 text-sm">No tenders yet.</p>
                <Link to="/tenders/new"
                      className="btn-primary mt-3 inline-flex text-sm">
                  Post your first tender
                </Link>
              </div>
            )}
            {tenders.map(t => (
              <div key={t.id} className="card flex items-center justify-between gap-4">
                <div className="min-w-0 flex-1">
                  <div className="flex items-center gap-2 mb-0.5">
                    <span className={statusBadge(t.status)}>{t.status}</span>
                    <span className="text-xs text-gray-400">{t.category}</span>
                  </div>
                  <p className="text-sm font-medium text-gray-900 truncate">
                    {t.title}
                  </p>
                  <p className="text-xs text-gray-400 mt-0.5">
                    {t.estimatedValue?.toLocaleString()} MDL
                    {t.deadline ? ` · Due ${fmtDate(t.deadline)}` : ''}
                  </p>
                </div>
                <div className="flex gap-2 shrink-0">
                  {t.status !== 'CANCELLED' && (
                    <Link to={`/tenders/${t.id}/offers`}
                          className="btn-secondary text-xs px-3 py-1.5">
                      Offers
                    </Link>
                  )}
                  <Link to={`/tenders/${t.id}`}
                        className="btn-secondary text-xs px-3 py-1.5">
                    View
                  </Link>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Right column: notifications + quarterly report */}
        <div className="space-y-4">
          {/* Notifications (drained from circular queue) */}
          <div>
            <h2 className="text-sm font-medium text-gray-700 mb-3">
              Notifications
              {notifs.length > 0 && (
                <span className="ml-2 text-xs bg-blue-100 text-blue-700
                                 px-1.5 py-0.5 rounded-full">
                  {notifs.length}
                </span>
              )}
            </h2>
            {notifs.length === 0
              ? <p className="text-xs text-gray-400">No new notifications.</p>
              : <div className="space-y-2">
                  {notifs.map(n => <Notification key={n.id} notif={n} />)}
                </div>
            }
          </div>

          {/* Quarterly report */}
          <div className="card">
            <h2 className="text-sm font-medium text-gray-700 mb-2">
              Quarterly report
            </h2>
            <p className="text-xs text-gray-500 mb-3">
              Generate your HG 870/2022 quarterly report for submission
              to the Public Procurement Agency.
            </p>
            <button onClick={generateReport}
                    disabled={reportLoading}
                    className="btn-secondary w-full text-sm">
              {reportLoading ? 'Generating…' : 'Generate Q report (N-ary tree)'}
            </button>
            {report && (
              <div className="mt-3 rounded-lg bg-green-50 border border-green-200
                              p-2 text-xs text-green-700">
                ✓ Report generated: {report.procedures} procedures,{' '}
                {report.totalValue?.toLocaleString()} MDL total.
                <p className="text-gray-400 mt-1">{report.reportPath}</p>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  )
}

/* ── Supplier dashboard ─────────────────────────────────────────── */
function SupplierDashboard({ user }) {
  const [tenders, setTenders] = useState([])
  const [notifs,  setNotifs]  = useState([])
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    Promise.all([
      api.get('/api/tenders?status=OPEN'),
      api.get('/api/notifications'),
    ]).then(([td, nd]) => {
      setTenders(td.tenders || [])
      setNotifs(nd.notifications || [])
    }).catch(console.error)
      .finally(() => setLoading(false))
  }, [])

  if (loading) return (
    <div className="text-center py-16 text-gray-400 text-sm">Loading…</div>
  )

  return (
    <div>
      <div className="mb-6">
        <h1 className="text-xl font-semibold text-gray-900">
          Dashboard — {user.name}
        </h1>
        <p className="text-sm text-gray-500 mt-0.5">
          Supplier · IDNO {user.idno}
        </p>
      </div>

      <div className="grid grid-cols-3 gap-6">
        {/* Open tenders */}
        <div className="col-span-2">
          <div className="flex items-center justify-between mb-3">
            <h2 className="text-sm font-medium text-gray-700">
              Open tenders ({tenders.length})
            </h2>
            <Link to="/tenders" className="text-xs text-blue-600 hover:underline">
              Browse all →
            </Link>
          </div>
          <div className="space-y-2">
            {tenders.length === 0 && (
              <div className="card text-center py-8">
                <p className="text-gray-400 text-sm">No open tenders at the moment.</p>
              </div>
            )}
            {tenders.map(t => (
              <div key={t.id} className="card flex items-center justify-between gap-4">
                <div className="min-w-0 flex-1">
                  <div className="flex items-center gap-2 mb-0.5">
                    <span className="badge-open">OPEN</span>
                    <span className="text-xs text-gray-400">{t.category}</span>
                  </div>
                  <p className="text-sm font-medium text-gray-900 truncate">
                    {t.title}
                  </p>
                  <p className="text-xs text-gray-400 mt-0.5">
                    Budget: {t.estimatedValue?.toLocaleString()} MDL
                    {t.deadline ? ` · Deadline: ${fmtDate(t.deadline)}` : ''}
                  </p>
                </div>
                <div className="flex gap-2 shrink-0">
                  <Link to={`/tenders/${t.id}/submit`}
                        className="btn-primary text-xs px-3 py-1.5">
                    Submit offer
                  </Link>
                  <Link to={`/tenders/${t.id}`}
                        className="btn-secondary text-xs px-3 py-1.5">
                    View
                  </Link>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Notifications */}
        <div>
          <h2 className="text-sm font-medium text-gray-700 mb-3">
            Notifications
            {notifs.length > 0 && (
              <span className="ml-2 text-xs bg-blue-100 text-blue-700
                               px-1.5 py-0.5 rounded-full">
                {notifs.length}
              </span>
            )}
          </h2>
          {notifs.length === 0
            ? <p className="text-xs text-gray-400">No new notifications.</p>
            : <div className="space-y-2">
                {notifs.map(n => <Notification key={n.id} notif={n} />)}
              </div>
          }
        </div>
      </div>
    </div>
  )
}

/* ── Main export ────────────────────────────────────────────────── */
export default function Dashboard() {
  const { user } = useAuth()
  if (!user) return null
  return user.role === 'AUTHORITY'
    ? <AuthorityDashboard user={user} />
    : <SupplierDashboard  user={user} />
}
