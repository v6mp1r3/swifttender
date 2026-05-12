import { Link, useNavigate } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'

/*
 * Navbar — top navigation bar.
 * Shows different links based on authentication state and user role.
 */
export default function Navbar() {
  const { user, logout } = useAuth()
  const navigate = useNavigate()

  const handleLogout = async () => {
    await logout()
    navigate('/login')
  }

  return (
    <nav className="bg-white border-b border-gray-200 sticky top-0 z-50">
      <div className="container mx-auto px-4 max-w-6xl">
        <div className="flex items-center justify-between h-14">

          {/* Brand */}
          <Link to="/tenders"
                className="flex items-center gap-2 font-semibold text-blue-600 text-lg">
            <svg className="w-6 h-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2}
                    d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
            </svg>
            SwiftTender
          </Link>

          {/* Nav links */}
          <div className="flex items-center gap-6 text-sm">
            <Link to="/tenders"
                  className="text-gray-600 hover:text-gray-900 transition-colors">
              Browse tenders
            </Link>

            {/* Authority-only links */}
            {user?.role === 'AUTHORITY' && (
              <Link to="/tenders/new"
                    className="text-gray-600 hover:text-gray-900 transition-colors">
                Post tender
              </Link>
            )}

            {/* Auth state */}
            {user ? (
              <div className="flex items-center gap-3">
                <Link to="/dashboard"
                      className="text-gray-600 hover:text-gray-900 transition-colors">
                  Dashboard
                </Link>
                <div className="h-4 w-px bg-gray-300" />
                <span className="text-gray-500 text-xs">{user.name}</span>
                <button onClick={handleLogout}
                        className="btn-secondary text-xs px-3 py-1.5">
                  Sign out
                </button>
              </div>
            ) : (
              <div className="flex items-center gap-2">
                <Link to="/login"    className="btn-secondary text-xs px-3 py-1.5">Sign in</Link>
                <Link to="/register" className="btn-primary  text-xs px-3 py-1.5">Register</Link>
              </div>
            )}
          </div>

        </div>
      </div>
    </nav>
  )
}
