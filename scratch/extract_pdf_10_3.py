import pypdf, sys

sys.stdout.reconfigure(encoding='utf-8')
reader = pypdf.PdfReader('Acuan/SPLN D3.006 2021 CSV.pdf')
print("=== PAGE 50 ===")
print(reader.pages[49].extract_text())
print("=== PAGE 51 ===")
print(reader.pages[50].extract_text())

