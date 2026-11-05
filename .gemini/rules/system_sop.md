# Mandatory Rule: Operating System & User Instructions Protocol

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
