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


# Task 06 — Preferencja GPU: iGPU / dGPU / RTX

## Cel

Dodaj zarządzanie preferencją GPU dla aplikacji/procesu przez ścieżkę `.exe`.

## Uczciwe ograniczenie

Nie obiecuj przełączania działającego procesu między iGPU i dGPU w locie.  
Windows zwykle zarządza tym jako preferencją dla aplikacji po ścieżce `.exe`, a efekt może wymagać ponownego uruchomienia programu.

UI ma wyraźnie pokazać tekst:

```text
GPU preference is stored for this executable path and may apply after restarting the target app.
```

Możesz przetłumaczyć to na polski w UI.

## Zakres

Dodaj moduł:

```text
src/core/GpuPreferenceManager.h
src/core/GpuPreferenceManager.cpp
```

## Preferencje

Obsłuż:

- Let Windows decide
- Power saving / iGPU
- High performance / dedicated GPU

## Rejestr

Użyj:

```text
HKCU\Software\Microsoft\DirectX\UserGpuPreferences
```

Wartość:

- nazwa wartości: pełna ścieżka `.exe`,
- dane string: zgodne z formatem Windows Graphics Settings.

Przed zapisem:

- odczytaj istniejącą wartość,
- zachowaj inne parametry w stringu, jeśli istnieją,
- zmień tylko `GpuPreference`,
- obsłuż brak ścieżki exe,
- obsłuż brak dostępu do rejestru.

## Adaptery GPU

Dodaj informacyjnie wykrywanie adapterów przez DXGI:

- nazwa adaptera,
- vendor id,
- dedicated video memory,
- czy wygląda na iGPU/dGPU według heurystyki.

Nie mapuj na siłę każdego adaptera do `GpuPreference`, jeśli Windows tego nie ujawnia jednoznacznie.  
W UI możesz pokazać:

```text
Detected GPUs:
- Intel UHD / likely power saving
- NVIDIA RTX / likely high performance
```

Ale nie udawaj stuprocentowej pewności.

## UI

W panelu szczegółów procesu dodaj sekcję:

```text
GPU Preference
Current preference: ...
Detected executable path: ...
[Let Windows decide] [Power saving/iGPU] [High performance/dGPU]
[Apply GPU preference]
Note: change may require app restart.
```

Jeśli proces nie ma znanej ścieżki `.exe`, sekcja ma być wyszarzona.

## Kryteria ukończenia

- Aplikacja czyta preferencję GPU dla procesu z poprawną ścieżką exe.
- Aplikacja zapisuje preferencję GPU do HKCU.
- UI pokazuje uczciwą informację o restarcie aplikacji.
- Adaptery DXGI są pokazane jako informacja pomocnicza.
- Nie ma obietnicy „live GPU switch”.
- `docs/SAFETY_NOTES.md` opisuje ograniczenia tej funkcji.

## Build i test

Uruchom build Debug i Release.  
Przetestuj na zwykłej aplikacji użytkownika z pełną ścieżką `.exe`.

Na końcu odpowiedzi podaj pełną ścieżkę do wygenerowanego pliku `.exe`.
