/*
 * Notification.jsx — renders a single notification entry.
 * Notifications are delivered from the backend circular queue
 * (dsa/queue.c) via GET /api/notifications.
 */

const icons = {
  WINNER:          '🏆',
  LOSER:           '📋',
  NEW_TENDER:      '📢',
  CONTRACT_READY:  '📝',
  DOC_REQUIRED:    '📎',
  SIGNED:          '✅',
}

const colors = {
  WINNER:          'border-l-green-500 bg-green-50',
  LOSER:           'border-l-gray-300 bg-gray-50',
  NEW_TENDER:      'border-l-blue-500 bg-blue-50',
  CONTRACT_READY:  'border-l-amber-500 bg-amber-50',
  DOC_REQUIRED:    'border-l-orange-500 bg-orange-50',
  SIGNED:          'border-l-green-500 bg-green-50',
}

const fmtTime = ts => new Date(ts * 1000).toLocaleString('en-GB', {
  day: '2-digit', month: 'short', hour: '2-digit', minute: '2-digit'
})

export default function Notification({ notif }) {
  return (
    <div className={`border-l-4 rounded-r-lg p-3 ${colors[notif.type] || 'border-l-gray-300 bg-gray-50'}`}>
      <div className="flex gap-2.5 items-start">
        <span className="text-lg mt-0.5">{icons[notif.type] || '🔔'}</span>
        <div className="flex-1 min-w-0">
          <p className="text-sm text-gray-800">{notif.message}</p>
          <p className="text-xs text-gray-400 mt-1">
            {fmtTime(notif.createdAt)} · Tender #{notif.tenderId}
          </p>
        </div>
      </div>
    </div>
  )
}
