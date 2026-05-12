import { createContext, useContext, useState, useEffect } from 'react'
import { api } from '../api/client'

/*
 * AuthContext — provides current user + auth helpers to the entire app.
 *
 * State is persisted in localStorage so sessions survive page refresh.
 * The token is sent as an Authorization header on every API request
 * (see api/client.js).
 */

const AuthContext = createContext(null)

export function AuthProvider({ children }) {
  const [user,    setUser]    = useState(null)
  const [loading, setLoading] = useState(true)  /* checking stored token */

  /* On mount: verify stored token against /api/auth/me */
  useEffect(() => {
    const token = localStorage.getItem('st_token')
    if (!token) { setLoading(false); return }

    api.get('/api/auth/me')
      .then(data => setUser(data.user))
      .catch(() => localStorage.removeItem('st_token'))
      .finally(() => setLoading(false))
  }, [])

  const login = async (email, password) => {
    const data = await api.post('/api/auth/login', { email, password })
    localStorage.setItem('st_token', data.token)
    setUser(data.user)
    return data.user
  }

  const logout = async () => {
    await api.post('/api/auth/logout', {}).catch(() => {})
    localStorage.removeItem('st_token')
    setUser(null)
  }

  const value = { user, loading, login, logout, setUser }

  return (
    <AuthContext.Provider value={value}>
      {children}
    </AuthContext.Provider>
  )
}

/* Hook for consuming auth context */
export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used inside <AuthProvider>')
  return ctx
}
