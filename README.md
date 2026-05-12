# SwiftTender

**A Digital Platform for Low-Value Public Procurement Management in the Republic of Moldova**

SwiftTender is a web-based procurement platform that guides contracting authorities and suppliers through the complete low-value public procurement workflow as defined by Government Decision No. 870/2022.

## Stack

- **Backend**: C (C11) + [mongoose.c](https://github.com/cesanta/mongoose) HTTP server
- **Frontend**: React + Vite
- **Persistence**: Binary file storage (custom DSA implementations)
- **Auth**: Session tokens (hash table backed)

## DSA Elements Implemented from Scratch

| Structure / Algorithm | Module | Purpose |
|---|---|---|
| Hash table (djb2 + open addressing) | `dsa/hash_table.c` | User session registry |
| Doubly linked list | `dsa/linked_list.c` | Tender catalogue |
| Min-heap | `dsa/heap.c` | Offer ranking by weighted score |
| Circular queue | `dsa/queue.c` | Notification system |
| Binary search | `dsa/binary_search.c` | Legal threshold lookup |
| N-ary tree | `dsa/tree.c` | Quarterly report generation |

## Legal Framework

Implements the procedure defined by:
- **Law No. 131/2015** on Public Procurement (Republic of Moldova)
- **Government Decision No. 870/2022** — Regulation on Low-Value Public Procurement

## Setup & Run

### Backend

```bash
cd backend
make
./swifttender
```

Server runs on `http://localhost:8000`

### Frontend

```bash
cd frontend
npm install
npm run dev
```

Frontend runs on `http://localhost:5173` (proxies `/api/*` to backend)

### Production build

```bash
cd frontend && npm run build
# backend serves frontend/dist/ as static files
cd backend && ./swifttender
```

## Project Structure

```
swifttender/
├── backend/
│   ├── main.c              # Entry point, mongoose event loop
│   ├── mongoose.h/c        # Embedded HTTP server
│   ├── router.c/h          # URL pattern → handler dispatch
│   ├── handlers/           # One file per API resource
│   ├── dsa/                # All data structures (from scratch)
│   ├── models/             # C struct definitions
│   ├── storage/            # File I/O layer
│   ├── utils/              # JSON builder, auth, threshold
│   └── data/               # Binary data files + uploads
└── frontend/
    └── src/
        ├── pages/          # Route-level React components
        ├── components/     # Reusable UI components
        └── api/            # All fetch() calls centralised
```

## Authors

- [Student 1 Name] — Backend DSA core, API handlers, storage layer
- [Student 2 Name] — Frontend, offer/contract/notification handlers, report generation

## Course

Data Structures and Algorithms — Technical University of Moldova, 2025–2026
