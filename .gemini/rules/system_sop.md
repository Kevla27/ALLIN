# Mandatory Rule: Operating System & User Instructions Protocol

0. **Core Identity (KITA ADALAH E3):**
   We are the development team and owners of **Pillar E3 (Smart Meter Application)**. E3 (`E3/app/`, `E3/include/`, `FlowChartE3.drawio`) is our primary product and core responsibility. E1 (Metrology) and E2 (DLMS) are integrated services connected through `meter_system_bridge.c`. Never sacrifice or corrupt E3 when handling integrations or external updates.

1. **Pre-Flight File Read:**
   At the beginning of every turn / command execution, you MUST read and follow `Acuan/SOP_SISTEM_KERJA_AI.md`.
2. **Git Commit & Push Mandate:**
   Never forget to push changes to git! Every time changes are made to code or documentation:
   - Document changes in `Changelog/XXX_*.md`.
   - Stage all relevant files (`git add`).
   - Commit with a clear message (`git commit`).
   - Run `git push origin main` and confirm push success.
3. **Reference & Flowchart Synchronization:**
   - Always keep all reference documents in `Acuan/` (`PIN.docx`, `pin_doc_extracted.txt`, `pin_doc_all_text.txt`, `KONFIGURASI_MX.md`, `ALLIN.ioc`) synchronized with code.
   - Flowchart must reflect working program logic ("menyesuaikan flowchart ke program").
4. **Zero-Tolerance Quality:**
   - Firmware must compile with 0 errors and 0 warnings (`ninja -C NonSecure/build`).
   - All unit tests must pass 100% (`ctest --test-dir E3/build`).

