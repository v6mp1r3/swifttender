/*
 * api/client.js — centralised HTTP client for SwiftTender.
 *
 * All API calls go through this module so:
 *  - Auth token is attached automatically from localStorage
 *  - Errors are parsed uniformly (throws Error with server message)
 *  - Base URL is a single constant to update
 *
 * Usage:
 *   import { api } from '../api/client'
 *   const tenders = await api.get('/api/tenders')
 *   const result  = await api.post('/api/tenders', { title, ... })
 */

const BASE = ''   /* same origin — Vite proxy handles /api/* in dev */

function token() {
  return localStorage.getItem('st_token')
}

async function request(method, path, body, isFormData = false) {
  const headers = {}

  if (token()) headers['Authorization'] = `Bearer ${token()}`
  if (body && !isFormData) headers['Content-Type'] = 'application/json'

  const res = await fetch(BASE + path, {
    method,
    headers,
    body: body
      ? (isFormData ? body : JSON.stringify(body))
      : undefined,
  })

  /* Parse response body */
  const text = await res.text()
  let data
  try { data = JSON.parse(text) } catch { data = { message: text } }

  if (!res.ok) {
    const msg = data?.error || data?.message || `HTTP ${res.status}`
    throw new Error(msg)
  }

  return data
}

export const api = {
  get:    (path)               => request('GET',    path),
  post:   (path, body)         => request('POST',   path, body),
  patch:  (path, body)         => request('PATCH',  path, body),
  delete: (path)               => request('DELETE', path),
  upload: (path, formData)     => request('POST',   path, formData, true),
}
