import { Routes, Route, Navigate } from 'react-router-dom'
import { useAuth } from './context/AuthContext'

import Navbar       from './components/Navbar'
import Login        from './pages/Login'
import Register     from './pages/Register'
import Dashboard    from './pages/Dashboard'
import TenderList   from './pages/TenderList'
import TenderDetail from './pages/TenderDetail'
import PostTender   from './pages/PostTender'
import SubmitOffer  from './pages/SubmitOffer'
import Offers       from './pages/Offers'
import Contract     from './pages/Contract'

/*
 * ProtectedRoute — redirects to /login if the user is not authenticated.
 * Optionally restricts to a specific role ('AUTHORITY' or 'SUPPLIER').
 */
function ProtectedRoute({ children, role }) {
  const { user, loading } = useAuth()

  if (loading) {
    return (
      <div className="min-h-screen flex items-center justify-center">
        <div className="text-gray-500 text-sm">Loading...</div>
      </div>
    )
  }

  if (!user) return <Navigate to="/login" replace />
  if (role && user.role !== role) return <Navigate to="/dashboard" replace />

  return children
}

export default function App() {
  return (
    <div className="min-h-screen flex flex-col">
      <Navbar />

      <main className="flex-1 container mx-auto px-4 py-8 max-w-6xl">
        <Routes>
          {/* Public */}
          <Route path="/"         element={<Navigate to="/tenders" replace />} />
          <Route path="/login"    element={<Login />} />
          <Route path="/register" element={<Register />} />
          <Route path="/tenders"  element={<TenderList />} />
          <Route path="/tenders/:id" element={<TenderDetail />} />

          {/* Protected — any authenticated user */}
          <Route path="/dashboard" element={
            <ProtectedRoute><Dashboard /></ProtectedRoute>
          } />

          {/* Protected — authority only */}
          <Route path="/tenders/new" element={
            <ProtectedRoute role="AUTHORITY"><PostTender /></ProtectedRoute>
          } />
          <Route path="/tenders/:id/offers" element={
            <ProtectedRoute role="AUTHORITY"><Offers /></ProtectedRoute>
          } />

          {/* Protected — supplier only */}
          <Route path="/tenders/:id/submit" element={
            <ProtectedRoute role="SUPPLIER"><SubmitOffer /></ProtectedRoute>
          } />

          {/* Protected — both roles (contract view) */}
          <Route path="/tenders/:id/contract" element={
            <ProtectedRoute><Contract /></ProtectedRoute>
          } />

          {/* 404 */}
          <Route path="*" element={
            <div className="text-center py-24">
              <p className="text-5xl font-light text-gray-300 mb-4">404</p>
              <p className="text-gray-500">Page not found</p>
            </div>
          } />
        </Routes>
      </main>

      <footer className="border-t border-gray-200 py-4 text-center text-xs text-gray-400">
        SwiftTender — Low-Value Public Procurement Platform · Republic of Moldova
      </footer>
    </div>
  )
}
