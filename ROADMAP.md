# ROADMAP

## Progress Overview
| Phase | Description | Status |
|---|---|---|
| Phase 1: Documentation | Defining concept, design and roadmap | ✅ |
| Phase 2: Infrastructure | Setting up CI/CD and development environment | ⏳ |
| Phase 3: Implementation | Core networking and web server logic | ⏳ |
| Phase 4: Finalization | Verification and documentation polishing | ⏳ |

## Goals
- [x] High-level concept definition ✅
- [x] Technical design specification ✅
- [ ] Automated CI/CD pipeline ⏳
- [ ] Functional Wi-Fi connectivity ⏳
- [ ] mDNS "mm.local" discovery ⏳
- [ ] Basic Web UI ⏳

## Phases

### Phase 1: Documentation
- [x] Create `CONCEPT.md` ✅
- [x] Create `DESIGN.md` ✅
- [x] Create `TOP_ARCHITECTURE.puml` ✅
- [x] Create `ROADMAP.md` ✅
- [x] Create `TECHNICAL_DEBTS.md` ✅
- [x] Create `README.md` ✅

### Phase 2: Infrastructure
- [ ] Setup GitHub Action workflows for CI/CD ⏳
    - [ ] Create basic GitHub Action workflow structure ⏳
    - [ ] Add documentation linting to CI ⏳
    - [ ] Integrate ESP-IDF build into CI ⏳
- [ ] Setup ESP-IDF test environment ⏳
    - [ ] Configure test framework ⏳
    - [ ] Setup emulation for automated testing ⏳
- [ ] Create `src/install.sh` ⏳
- [ ] Create `test/install.sh` ⏳

### Phase 3: Implementation
- [ ] Implement Network Manager (Wi-Fi station) ⏳
- [ ] Implement Discovery Service (mDNS) ⏳
- [ ] Implement Web Service (HTTP server) ⏳

### Phase 4: Finalization
- [ ] Final PoC verification ⏳
- [ ] Generate API Documentation (Redocly) ⏳
- [ ] Review technical debts ⏳
