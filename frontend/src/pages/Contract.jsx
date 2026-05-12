import { useState, useEffect } from 'react'
import { useParams, Link } from 'react-router-dom'
import { api } from '../api/client'
import { useAuth } from '../context/AuthContext'

const statusInfo = {
  DRAFT:       { label: 'Draft — signatures pending', color: 'text-amber-600', bg: 'bg-amber-50 border-amber-200' },
  SIGNED_AUTH: { label: 'Signed by authority — awaiting supplier', color: 'text-blue-600', bg: 'bg-blue-50 border-blue-200' },
  SIGNED_BOTH: { label: 'Fully signed', color: 'text-green-600', bg: 'bg-green-50 border-green-200' },
  COMPLETED:   { label: 'Completed', color: 'text-gray-600', bg: 'bg-gray-50 border-gray-200' },
}

export default function Contract() {
  const { id }   = useParams()
  const { user } = useAuth()

  const [contract,  setContract]  = useState(null)
  const [loading,   setLoading]   = useState(true)
  const [error,     setError]     = useState('')
  const [signing,   setSigning]   = useState(false)
  const [signed,    setSigned]    = useState(false)
  const [uploading, setUploading] = useState(false)
  const [docSent,   setDocSent]   = useState(false)

  useEffect(() => {
    api.get(`/api/tenders/${id}/contract`)
      .then(d => setContract(d))
      .catch(e => setError(e.message))
      .finally(() => setLoading(false))
  }, [id])

  const handleSign = async () => {
    setSigning(true)
    try {
      await api.post(`/api/tenders/${id}/sign`, {})
      setSigned(true)
      setContract(c => ({ ...c, status: 'SIGNED_BOTH' }))
    } catch (err) {
      setError(err.message)
    } finally {
      setSigning(false)
    }
  }

  const handleDocUpload = async e => {
    const file = e.target.files[0]
    if (!file) return
    setUploading(true)
    try {
      const fd = new FormData()
      fd.append('file', file)
      await api.upload(`/api/tenders/${id}/documents`, fd)
      setDocSent(true)
    } catch (err) {
      setError(err.message)
    } finally {
      setUploading(false)
    }
  }

  if (loading) return (
    <div className="text-center py-16 text-gray-400 text-sm">Loading contract…</div>
  )

  if (error && !contract) return (
    <div className="card border-red-200 bg-red-50 text-sm text-red-700">
      {error} — <Link to={`/tenders/${id}`} className="underline">back to tender</Link>
    </div>
  )

  const info   = statusInfo[contract?.status] || statusInfo.DRAFT
  const isAuth = user?.role === 'AUTHORITY'

  return (
    <div className="max-w-2xl mx-auto">
      <Link to={`/tenders/${id}`}
            className="inline-flex items-center gap-1.5 text-sm text-gray-500
                       hover:text-gray-800 mb-6 transition-colors">
        ← Tender detail
      </Link>

      <div className="mb-6">
        <h1 className="text-xl font-semibold text-gray-900">Contract</h1>
        <p className="text-sm text-gray-500 mt-0.5">Tender #{id}</p>
      </div>

      {/* Status card */}
      <div className={`card mb-4 ${info.bg}`}>
        <div className="flex items-center gap-3">
          <div className="text-2xl">
            {contract?.status === 'SIGNED_BOTH' ? '✅' :
             contract?.status === 'DRAFT'       ? '📄' : '✍️'}
          </div>
          <div>
            <p className={`text-sm font-medium ${info.color}`}>{info.label}</p>
            <p className="text-xs text-gray-500 mt-0.5">
              Contract document generated via N-ary tree (SwiftTender DSA)
            </p>
          </div>
        </div>
      </div>

      {/* Contract file path */}
      {contract?.contractPath && (
        <div className="card mb-4">
          <p className="text-xs text-gray-400 mb-1">Document path</p>
          <p className="text-sm font-mono text-gray-700 break-all">
            {contract.contractPath}
          </p>
          <p className="text-xs text-gray-400 mt-2">
            The contract was generated using a hierarchical N-ary tree
            (dsa/tree.c) with pre-order traversal to emit structured sections.
          </p>
        </div>
      )}

      {/* MSign signing */}
      {!signed && contract?.status !== 'SIGNED_BOTH' && (
        <div className="card mb-4">
          <h2 className="text-sm font-medium text-gray-700 mb-3">
            Electronic signature required
          </h2>
          <p className="text-sm text-gray-500 mb-4">
            Both parties must sign the contract electronically using
            Moldova's MSign qualified electronic signature service before
            the contract is legally binding.
          </p>
          <button
            onClick={handleSign}
            disabled={signing}
            className="btn-primary w-full"
          >
            {signing ? 'Processing…' : '✍️ Sign with MSign'}
          </button>
          <p className="text-xs text-gray-400 mt-2 text-center">
            MSign integration is simulated in this prototype.
            Production would redirect to sign.gov.md.
          </p>
        </div>
      )}

      {/* Success: signed */}
      {signed && (
        <div className="card mb-4 bg-green-50 border-green-200">
          <p className="text-sm font-medium text-green-800">
            ✅ Contract signed successfully (MSign mock)
          </p>
          <p className="text-xs text-green-700 mt-1">
            Both parties have been notified via the notification queue.
          </p>
        </div>
      )}

      {/* Supplier: upload final documents */}
      {!isAuth && (
        <div className="card mb-4">
          <h2 className="text-sm font-medium text-gray-700 mb-3">
            Final delivery documents
          </h2>
          <p className="text-sm text-gray-500 mb-3">
            After delivering the goods/services, upload your fiscal invoice
            and delivery confirmation (act de recepție).
          </p>

          {docSent ? (
            <div className="rounded-lg bg-green-50 border border-green-200 p-3
                            text-sm text-green-700">
              ✓ Documents uploaded — authority has been notified.
            </div>
          ) : (
            <>
              <input type="file" accept=".pdf,.doc,.docx,.jpg,.png"
                className="block w-full text-sm text-gray-500
                           file:mr-3 file:py-1.5 file:px-3 file:rounded-lg
                           file:border file:border-gray-300 file:text-sm
                           file:bg-white file:text-gray-700 hover:file:bg-gray-50"
                disabled={uploading}
                onChange={handleDocUpload} />
              {uploading && (
                <p className="text-xs text-blue-500 mt-2">Uploading…</p>
              )}
            </>
          )}
        </div>
      )}

      {error && (
        <div className="card border-red-200 bg-red-50 text-sm text-red-700">
          {error}
        </div>
      )}
    </div>
  )
}
