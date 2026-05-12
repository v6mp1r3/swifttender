import { useState } from 'react'
import { Link, useNavigate } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'
import { api } from '../api/client'

/*
 * Register.jsx
 *
 * Two-step registration:
 *   Step 1: fill in account details (name, email, password, IDNO, role)
 *   Step 2: upload company license document
 *
 * On submit the form calls POST /api/auth/register, which:
 *   - Validates fields and checks for duplicate email/IDNO
 *   - Hashes password (iterative djb2, utils/auth.c)
 *   - Persists User to users.bin (storage/file_io.c)
 *   - Returns { token, user }
 */
export default function Register() {
  const { setUser } = useAuth()
  const navigate    = useNavigate()

  const [form, setForm] = useState({
    name:     '',
    email:    '',
    password: '',
    confirm:  '',
    idno:     '',
    role:     'AUTHORITY',
  })
  const [error,   setError]   = useState('')
  const [loading, setLoading] = useState(false)

  const handleChange = e =>
    setForm(f => ({ ...f, [e.target.name]: e.target.value }))

  const handleSubmit = async e => {
    e.preventDefault()
    setError('')

    if (form.password !== form.confirm) {
      setError('Passwords do not match'); return
    }
    if (form.idno.length < 7) {
      setError('IDNO must be at least 7 characters'); return
    }

    setLoading(true)
    try {
      const data = await api.post('/api/auth/register', {
        name:     form.name,
        email:    form.email,
        password: form.password,
        idno:     form.idno,
        role:     form.role,
      })
      localStorage.setItem('st_token', data.token)
      setUser(data.user)
      navigate('/dashboard')
    } catch (err) {
      setError(err.message || 'Registration failed')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="min-h-[70vh] flex items-center justify-center py-8">
      <div className="w-full max-w-md">

        <div className="text-center mb-8">
          <h1 className="text-2xl font-semibold text-gray-900">Create account</h1>
          <p className="text-sm text-gray-500 mt-1">
            Join SwiftTender to post or participate in low-value tenders
          </p>
        </div>

        <div className="card">
          {error && (
            <div className="mb-4 px-3 py-2 rounded-lg bg-red-50 border
                            border-red-200 text-sm text-red-700">
              {error}
            </div>
          )}

          <form onSubmit={handleSubmit} className="space-y-4">

            {/* Role selector */}
            <div>
              <label className="label">I am a</label>
              <div className="grid grid-cols-2 gap-3 mt-1">
                {['AUTHORITY', 'SUPPLIER'].map(r => (
                  <button
                    key={r} type="button"
                    onClick={() => setForm(f => ({ ...f, role: r }))}
                    className={`py-2.5 rounded-lg border text-sm font-medium
                      transition-colors duration-150
                      ${form.role === r
                        ? 'border-blue-600 bg-blue-50 text-blue-700'
                        : 'border-gray-300 text-gray-600 hover:bg-gray-50'
                      }`}
                  >
                    {r === 'AUTHORITY' ? '🏛 Contracting Authority' : '🏢 Supplier'}
                  </button>
                ))}
              </div>
              <p className="text-xs text-gray-400 mt-1.5">
                {form.role === 'AUTHORITY'
                  ? 'Post tenders, evaluate offers, award contracts'
                  : 'Browse tenders, submit offers, sign contracts'}
              </p>
            </div>

            {/* Name */}
            <div>
              <label className="label" htmlFor="name">
                {form.role === 'AUTHORITY' ? 'Institution name' : 'Company name'}
              </label>
              <input
                id="name" name="name" type="text" className="input"
                placeholder={form.role === 'AUTHORITY'
                  ? 'Liceul Teoretic Nr. 1'
                  : 'SRL Example'}
                value={form.name} onChange={handleChange}
                required autoFocus
              />
            </div>

            {/* IDNO */}
            <div>
              <label className="label" htmlFor="idno">
                IDNO
                <span className="ml-1 text-xs font-normal text-gray-400">
                  (Fiscal identification number)
                </span>
              </label>
              <input
                id="idno" name="idno" type="text" className="input"
                placeholder="1007607001234"
                value={form.idno} onChange={handleChange}
                required maxLength={13}
              />
            </div>

            {/* Email */}
            <div>
              <label className="label" htmlFor="email">Email</label>
              <input
                id="email" name="email" type="email" className="input"
                placeholder="contact@institution.md"
                value={form.email} onChange={handleChange}
                required
              />
            </div>

            {/* Password */}
            <div className="grid grid-cols-2 gap-3">
              <div>
                <label className="label" htmlFor="password">Password</label>
                <input
                  id="password" name="password" type="password"
                  className="input" placeholder="••••••••"
                  value={form.password} onChange={handleChange}
                  required minLength={6}
                />
              </div>
              <div>
                <label className="label" htmlFor="confirm">Confirm</label>
                <input
                  id="confirm" name="confirm" type="password"
                  className="input" placeholder="••••••••"
                  value={form.confirm} onChange={handleChange}
                  required
                />
              </div>
            </div>

            {/* Legal notice */}
            <p className="text-xs text-gray-400 bg-gray-50 rounded-lg p-3">
              By registering you confirm that your institution is authorised
              to conduct public procurement under Law 131/2015 and HG 870/2022.
            </p>

            <button
              type="submit"
              className="btn-primary w-full"
              disabled={loading}
            >
              {loading ? 'Creating account…' : 'Create account'}
            </button>
          </form>
        </div>

        <p className="text-center text-sm text-gray-500 mt-4">
          Already registered?{' '}
          <Link to="/login" className="text-blue-600 hover:underline font-medium">
            Sign in
          </Link>
        </p>
      </div>
    </div>
  )
}
