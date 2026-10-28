import pypdf, sys

sys.stdout.reconfigure(encoding='utf-8')
reader = pypdf.PdfReader('Acuan/SPLN D3.006 2021 CSV.pdf')
for i, page in enumerate(reader.pages):
    text = page.extract_text() or ""
    if 'Tabel 6' in text or 'Tabel 6.' in text:
        print(f"=== PAGE {i+1} ===")
        print(text)

