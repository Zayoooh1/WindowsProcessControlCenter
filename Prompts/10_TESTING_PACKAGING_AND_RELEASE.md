# Wspólne założenia projektu

Projekt: **Windows Process Control Center**  
Język: **C++20**  
Platforma: **Windows 10/11 x64**  
Build: **CMake + MSVC / Visual Studio 2022**  
GUI: **Win32 + DirectX 11 + Dear ImGui**, bez klasycznych kontrolek Win32 wyglądających jak Windows XP.  
Styl UI: nowoczesny, ciemny, spokojny, czytelny, bez przeładowania elementami.

## Najważniejsze zasady

- To ma być normalne narzędzie administracyjne, nie malware.
- Nie dodawaj ukrywania procesów, injection, keyloggerów, driverów kernela, omijania zabezpieczeń ani funkcji stealth.
- Operacje destrukcyjne, takie jak zakończenie procesu lub priorytet czasu rzeczywistego, wymagają czytelnego ostrzeżenia.
- Aplikacja nie może „udawać”, że zrobiła coś, czego Windows realnie nie pozwala zrobić.
- Preferencja GPU ma być pokazana uczciwie: Windows zwykle ustawia ją per ścieżka `.exe`, a efekt może wymagać restartu aplikacji/procesu.
- Dla procesów systemowych, chronionych, CSRSS, Idle, services hostów i własnego procesu aplikacji akcje ryzykowne mają być domyślnie zablokowane albo wymagać bardzo wyraźnego potwierdzenia.
- Każdy etap musi budować się poprawnie.
- Po każdej większej zmianie uruchom build i zostaw krótką notatkę w `docs/PROGRESS.md`.

## Minimalna struktura repozytorium

```text
WindowsProcessControlCenter/
  CMakeLists.txt
  README.md
  docs/
    ARCHITECTURE.md
    PROGRESS.md
    SAFETY_NOTES.md
  src/
    main.cpp
    app/
      Application.h
      Application.cpp
    core/
      ProcessInfo.h
      ProcessEnumerator.h
      ProcessEnumerator.cpp
      ProcessActions.h
      ProcessActions.cpp
      PriorityManager.h
      PriorityManager.cpp
      SuspensionManager.h
      SuspensionManager.cpp
      GpuPreferenceManager.h
      GpuPreferenceManager.cpp
      PrivilegeHelper.h
      PrivilegeHelper.cpp
      ErrorUtils.h
      ErrorUtils.cpp
    ui/
      UiShell.h
      UiShell.cpp
      ProcessTableView.h
      ProcessTableView.cpp
      Theme.h
      Theme.cpp
    platform/
      WinHandles.h
      WinHandles.cpp
  third_party/
    imgui/
```


# Task 10 — Testy, packaging i release

## Cel

Przygotuj projekt tak, żeby dało się go normalnie zbudować, przetestować i wypuścić jako MVP.

## Zakres

Dodaj:

- instrukcję build w README,
- skrypt PowerShell do builda,
- skrypt PowerShell do czyszczenia builda,
- podstawowe testy jednostkowe dla logiki niezależnej od WinAPI,
- checklistę testów manualnych,
- konfigurację Release.

## Pliki

```text
scripts/build.ps1
scripts/clean.ps1
docs/MANUAL_TEST_CHECKLIST.md
docs/RELEASE_NOTES_TEMPLATE.md
tests/
  CMakeLists.txt
  TestPriorityMapping.cpp
  TestGpuPreferenceString.cpp
  TestSafetyRules.cpp
```

## Testy jednostkowe

Przetestuj minimum:

- mapowanie priorytetów UI ↔ WinAPI,
- parsowanie i modyfikację stringa `GpuPreference`,
- reguły bezpieczeństwa `CanPerformAction`,
- formatowanie błędów WinAPI, jeśli da się bez mockowania systemu.

## Manual test checklist

Uwzględnij:

- uruchomienie bez admina,
- uruchomienie jako admin,
- enumerację procesów,
- terminate `notepad.exe`,
- priority change `notepad.exe`,
- freeze/resume `notepad.exe`,
- GPU preference dla zwykłej aplikacji,
- DPI 100/125/150%,
- brak crasha przy procesach, które znikają,
- zachowanie po uszkodzonym settings.json.

## Packaging

Przygotuj folder:

```text
dist/WindowsProcessControlCenter/
```

Ma zawierać:

- `.exe`,
- wymagane DLL/biblioteki, jeśli są,
- README,
- SAFETY_NOTES,
- LICENSE placeholder, jeśli projektu nie ma jeszcze na konkretnej licencji.

Nie rób instalatora MSI na tym etapie, chyba że projekt już jest stabilny.

## Kryteria ukończenia

- `scripts/build.ps1` buduje projekt.
- Testy przechodzą.
- Folder `dist/` zawiera działające MVP.
- README pozwala odpalić projekt bez zgadywania.
- `docs/MANUAL_TEST_CHECKLIST.md` jest konkretna.

## Build i test

Uruchom:

```powershell
./scripts/build.ps1 -Configuration Release
ctest --test-dir build --output-on-failure
```

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
