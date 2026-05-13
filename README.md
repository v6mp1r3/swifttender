# SwiftTender

**A Digital Platform for Low-Value Public Procurement Management in the Republic of Moldova**

> Individual Work — Data Structures and Algorithms Course  
> Technical University of Moldova, Faculty of Computers, Informatics and Microelectronics, 2025–2026

---

## What is SwiftTender?

SwiftTender is a full-stack web application that digitises the low-value public procurement process in Moldova. It guides contracting authorities and suppliers through the complete workflow defined by Government Decision No. 870/2022 — from posting a tender to signing a contract.

Moldova has MTender for above-threshold competitive tenders, but no dedicated digital tool exists for low-value purchases (below 300,000 MDL for goods, 375,000 MDL for works). Civil society monitoring found that 96% of this spending category operates without structured oversight. SwiftTender fills this gap.

**Live demo:** [https://swifttender-production.up.railway.app](https://swifttender-production-f1f8.up.railway.app/tenders)

---

## Features

### For Contracting Authorities
- Post tenders with legal threshold enforcement (HG 870/2022)
- Set evaluation criteria and scoring weights (price vs. delivery)
- View supplier offers ranked automatically by weighted score
- Select winner with mandatory justification for non-top-ranked selections
- Generate quarterly reports for submission to the Public Procurement Agency
- Sign contracts electronically (MSign integration)

### For Suppliers
- Browse and filter open tenders
- Submit offers with required documentation
- Receive notifications on winner selection and contract status
- Upload final delivery documents

### System
- Automatic threshold check: redirects to MTender if value exceeds low-value limit
- Immutable audit log of all procurement actions
- Role-based access control (Authority / Supplier)

---

## DSA Elements — Implemented from Scratch in C

All data structures and algorithms are hand-implemented with no external libraries.

| Structure / Algorithm | Module | Used for | Complexity |
|---|---|---|---|
| **Hash table** (djb2 + open addressing) | `dsa/hash_table.c` | Session token registry — O(1) auth on every request | O(1) avg lookup |
| **Doubly linked list** | `dsa/linked_list.c` | Tender catalogue — O(1) insert and remove | O(1) insert/remove |
| **Min-heap** | `dsa/heap.c` | Offer ranking by weighted score (price + delivery) | O(log n) insert/extract |
| **Circular queue** | `dsa/queue.c` | Per-user notification FIFO buffer | O(1) enqueue/dequeue |
| **Binary search** | `dsa/binary_search.c` | Legal threshold lookup on sorted category table | O(log n) |
| **N-ary tree** | `dsa/tree.c` | Quarterly report generation via pre-order traversal | O(n) traversal |

### Scoring Formula (Min-Heap ranking)
```
price_norm    = offer.price        / min_price_in_set
delivery_norm = offer.delivery_days / min_days_in_set
key = (price_weight / 100) * price_norm
    + (delivery_weight / 100) * delivery_norm
```
Lower key = better offer. The heap root is always the most advantageous offer.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Backend | C (C11) + [mongoose.c](https://github.com/cesanta/mongoose) embedded HTTP server |
| Frontend | React 18 + Vite + Tailwind CSS |
| Persistence | Binary file storage (flat struct arrays, fread/fwrite) |
| Auth | Session tokens stored in hash table (no external auth library) |
| Deployment | Docker + Railway |

---

## Legal Framework

- **Law No. 131/2015** on Public Procurement (Republic of Moldova)
- **Government Decision No. 870/2022** — Regulation on Low-Value Public Procurement (in force July 1, 2023)
- Thresholds: Goods/Services ≤ 300,000 MDL · Works ≤ 375,000 MDL · Social Services ≤ 600,000 MDL

---

## Project Structure

```
swifttender/
├── Dockerfile                  # Production build (Ubuntu + Node + GCC)
├── railway.toml                # Railway deployment config
│
├── backend/                    # C backend
│   ├── main.c                  # Entry point, mongoose event loop, SPA fallback
│   ├── mongoose.h / mongoose.c # Embedded HTTP server (download separately)
│   ├── router.c / router.h     # URL dispatch with custom uri_match()
│   ├── Makefile
│   │
│   ├── dsa/                    # ★ All DSA implementations
│   │   ├── hash_table.c/h      # Open addressing, djb2, tombstone deletion
│   │   ├── linked_list.c/h     # Doubly linked, O(1) remove
│   │   ├── heap.c/h            # Min-heap, sift-up/sift-down
│   │   ├── queue.c/h           # Circular buffer, wrap-around arithmetic
│   │   ├── binary_search.c/h   # Divide and conquer, overflow-safe midpoint
│   │   └── tree.c/h            # N-ary tree, pre-order traversal
│   │
│   ├── handlers/               # One file per API resource
│   │   ├── auth_handler.c      # POST /api/auth/register|login, GET /me
│   │   ├── tender_handler.c    # CRUD /api/tenders
│   │   ├── offer_handler.c     # /api/tenders/:id/offers + winner
│   │   ├── contract_handler.c  # Contract generation + MSign mock
│   │   ├── notify_handler.c    # GET /api/notifications
│   │   └── report_handler.c    # Quarterly report + file upload
│   │
│   ├── models/                 # C struct definitions (User, Tender, Offer, ...)
│   ├── storage/                # Binary file I/O (file_io.c, upload.c)
│   ├── utils/                  # json.c, auth.c, threshold.c, ranking.c, notify.c
│   └── data/                   # Runtime data (binary files, uploads, reports)
│       └── thresholds.csv      # Legal limits — editable without recompile
│
└── frontend/                   # React frontend
    └── src/
        ├── pages/              # Login, Register, Dashboard, TenderList,
        │                       # TenderDetail, PostTender, SubmitOffer,
        │                       # Offers, Contract
        ├── components/         # Navbar, RankingTable, ThresholdBadge, Notification
        ├── api/client.js       # Centralised fetch() with auto Bearer token
        └── context/AuthContext # Global auth state + token persistence
```

---

## REST API

| Method | Route | Auth | Description |
|---|---|---|---|
| POST | `/api/auth/register` | — | Register authority or supplier |
| POST | `/api/auth/login` | — | Login, receive session token |
| GET | `/api/auth/me` | ✓ | Get current user |
| GET | `/api/tenders` | — | List tenders (filter by status/category) |
| POST | `/api/tenders` | Authority | Post new tender (threshold checked) |
| GET | `/api/tenders/:id` | — | Get tender details |
| PATCH | `/api/tenders/:id` | Authority | Update tender |
| DELETE | `/api/tenders/:id` | Authority | Cancel tender |
| GET | `/api/tenders/:id/offers` | Authority | Get ranked offers (heap-sorted) |
| POST | `/api/tenders/:id/offers` | Supplier | Submit offer |
| POST | `/api/tenders/:id/winner` | Authority | Select winner |
| GET | `/api/tenders/:id/contract` | Both | Get/generate contract |
| POST | `/api/tenders/:id/sign` | Both | Sign contract (MSign mock) |
| POST | `/api/tenders/:id/documents` | Supplier | Upload delivery docs |
| GET | `/api/notifications` | ✓ | Get pending notifications |
| GET | `/api/reports/quarterly` | Authority | Generate quarterly report |
| POST | `/api/uploads` | ✓ | Upload file |

---

## Running Locally

### Prerequisites
- GCC 9+ or Clang 10+
- GNU Make
- Node.js 18+
- curl (to download mongoose)

### Step 1 — Download mongoose
```bash
cd swifttender/backend
curl -O https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h
curl -O https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c
```

### Step 2 — Build and run the backend
```bash
cd swifttender/backend
make
./swifttender
# Listening on http://0.0.0.0:8000
```

### Step 3 — Run the frontend
```bash
cd swifttender/frontend
npm install
npm run dev
# Open http://localhost:5173
```

### Demo walkthrough
1. Register as **Contracting Authority** (role: AUTHORITY)
2. Post a tender — watch the threshold badge update live
3. Open an incognito window, register as **Supplier**
4. Browse tenders → Submit an offer with price and delivery
5. Back in authority window → View offers → See heap-ranked results
6. Select winner → notification fires
7. Both parties sign contract (MSign mock)
8. Supplier uploads delivery documents
9. Authority generates quarterly report (N-ary tree output)

---

## Deployment (Docker / Railway)

The app is containerised and deploys automatically from GitHub via Railway.

```bash
# Build locally with Docker
docker build -t swifttender .
docker run -p 8000:8000 -e PORT=8000 swifttender
```

Environment variables:
| Variable | Default | Description |
|---|---|---|
| `PORT` | `8000` | HTTP port (Railway sets this automatically) |
| `STATIC_DIR` | `../frontend/dist` | Path to built React files |

---

## Authors

- **Rusnac Adelina** — Backend DSA core, API handlers (auth, tenders), storage layer, file I/O
- **Guțu Ana** — Frontend (React), offer/contract/notification handlers, report generation

---

## Acknowledgements

- [mongoose.c](https://github.com/cesanta/mongoose) — embedded HTTP server by Cesanta (MIT license)
- [Open Contracting Partnership](https://www.open-contracting.org) — OCDS standard and Moldova procurement data
- [AGER Moldova](https://ager.md) — civil society monitoring reports on low-value procurement
